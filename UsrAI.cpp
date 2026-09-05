#include "UsrAI.h"
#include<set>
#include <iostream>
#include<unordered_map>
#include<list>
#include <cstdlib>

using namespace std;
tagGame tagUsrGame;
ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/
#define MAP_SIZE 100
#define stageExplore 1
#define stageDefense1 2
#define stageDefense2 3
#define stageAttack 4
static int stage=1;
static int terrainCache[MAP_SIZE][MAP_SIZE];

//距离计算函数
static double calDistance(double dr1,double ur1,double dr2,double ur2){
    double ddr=dr1-dr2;
    double dur=ur1-ur2;
    return sqrt(ddr*ddr+dur*dur)
}
//块坐标转换为细节坐标
static double blockToDetail(int block) {
    return block * BLOCKSIDELENGTH + BLOCKSIDELENGTH / 2.0;
}
//地图缓存更新
static void updateTerrainCache(const tagInfo& info) {
    for (int i=0;i<MAP_SIZE;i++){
        for(int j=0;j<MAP_SIZE;j++){ terrainCache[i][j]=-1;}// 初始化所有格子为 -1
    }
    if(info.theMap!=0){
        for(int i=0;i<MAP_SIZE;i++){
            for(int j=0;j<MAP_SIZE;j++){
        tagTerrain& t=(*info.theMap)[i][j];
        if(t.type==MAPPATTERN_GRASS&&t.height>=0){terrainCache[i][j]=0;}
            }
        }
    }// 标记空地(0)
    for(tagBuilding& b:info.buildings){
        int size = 2;
        switch (b.Type) {
            case BUILDING_HOME:      size = 2; break;
            case BUILDING_ARROWTOWER:size = 2; break;
            case BUILDING_FARM:      size = 3; break;
            case BUILDING_CENTER:    size = 3; break;
            case BUILDING_STOCK:     size = 3; break;
            case BUILDING_GRANARY:   size = 3; break;
            case BUILDING_ARMYCAMP:  size = 3; break;
            case BUILDING_RANGE:     size = 3; break;
            case BUILDING_STABLE:    size = 3; break;
            case BUILDING_MARKET:    size = 3; break;
            case BUILDING_COLLAGE:   size = 3; break;
            case BUILDING_SIEGE:     size = 3; break;
            default:                 size = 2; break;
            }
        for(int i=b.BlockDR;i<=b.BlockDR+size;i++){
            for(int j=b.BlockUR;j<=b.BlockUR+size;j++){
                terrainCache[i][j]=1;
            }
        }  
    }//标记建筑(1)
    for(tagResource& r:info.resources){
        if(r.BlockDR>=0&&r.BlockDR<MAP_SIZE&&r.BlockUR>=0&&r.BlockUR<MAP_SIZE)
        {terrainCache[r.BlockDR][r.BlockUR]=2;}
        }//标记资源(2)
    for(tagFarmer& f=info.farmers){
        if(f.BlockDR>=0&&f.BlockDR<MAP_SIZE&&f.BlockUR>=0&&f.BlockDR<MAP_SIZE){
         terrainCache[f.BlockDR][f.BlockUR]=3;   
        }
     }
    for(tagArmy& a=info.armies){
        if(a.BlockDR>=0&&a.BlockDR<MAP_SIZE&&a.BlockUR>=0&&a.BlockDR<MAP_SIZE){
         terrainCache[a.BlockDR][a.BlockUR]=3;   
        }
     }//标记单位(3)
}
//找空地
static bool findEmptyBlock(int& outDR, int& outUR, int size) {
        for(int i=0;i<MAP_SIZE;i++){
            for(int j=0;j<MAP_SIZE;j++){
                if(terrainCache[i][j]==0){
                 outDR=i;
                 outUR=j;
                 return true;
                }
            }
         }
        return false;
    }
//阶段切换
static void updateStage(tagInfo& info) {
    int frame=info.GameFrame;
    if(frame<6000) {
        stage=stageExplore;
    } else if(frame<13500) {
        stage=stageDefense1;
    } else if(frame<21000) {
        stage=stageDefense2;
    } else {
        stage=stageAttack;
    }
}
//采集：砍树，采浆果，采金矿，挖石头
static void cutTree(tagInfo& info,int num,int resourceType) {
    int count=0;
    for(tagFarmer& f:info.farmers){
        if(f.FarmerSort!=FARMERTYPE_FARMER) continue;
        if(f.Blood<=0) continue;
        if(f.NowState!=HUMAN_STATE_IDLE) continue;
        int targetSN=-1;
        double minDist=1e9;
        for(tagResource& r:info.resources){
            if(r.Type!=resourceType) continue;
            if(r.Blood<=0&&r.Cnt<=0) continue;
            double dist=calDistance(f.DetailDR,f.DetailUR,r.DetailDR,r.DetailUR);
            if(dist<minDist){
                minDist=dist;
                targetSN=r.SN;
            }
        }
        if(targetSN!=-1){
            HumanAction(f.SN,targetSN);
            count++;
        }
        if(count>=num) break;
    }    
}
//打猎：羚羊
static void hunting(tagInfo& info, int targetCount) {
    if (targetCount <= 0) return;
    int assigned = 0;
    vector<tagResource*> gazelles;
    for (tagResource& r : info.resources) {
        if (r.Type == RESOURCE_GAZELLE && (r.Blood > 0 || r.Cnt > 0)) {
            gazelles.push_back(&r);
        }
    }
    if (gazelles.empty()) return;
    for (tagFarmer& f : info.farmers) {
        if (f.FarmerSort != FARMERTYPE_FARMER) continue;
        if (f.Blood <= 0) continue;
        if (f.NowState != HUMAN_STATE_IDLE) continue;
        int targetSN = -1;
        double minDist = 1e9;
        for (tagResource* g : gazelles) {
            double d = calDistance(f.DR, f.UR, g->DR, g->UR);
            if (d < minDist) { minDist = d; targetSN = g->SN; }
        }
        if (targetSN == -1) return;
        HumanAction(f.SN, targetSN);
        assigned++;
        if (assigned >= targetCount) break;
    }
}
//建筑：市镇中心，谷仓，市场，农田，兵营、靶场 、马厩
static void buildBuilding(tagInfo& info,int buildingType,int num){
    int size=3;
    if(buildingType==BUILDING_HOME||buildingType==BUILDING_ARROWTOWER) size=2;
    int currentCount=0;
    int buildDR,buildUR;
    if(!findEmptyBlock(buildDR,buildUR,size))  return;
    for(tagFarmer& f:info.farmers){
        if(f.FarmerSort!=FARMERTYPE_FARMER) continue;
        if(f.Blood<=0) continue;
        if(f.NowState!=HUMAN_STATE_IDLE) continue;
        currentCount++;
        if(currentCount>num) break;
        HumanBuild(f.SN,buildingType,buildDR,buildUR);
     }
}
//军队管理
static void armymanage(tagInfo& info){
    for(tagArmy& a:info.armies){
        if(a.sort==AT_PRIEST) continue;
        if(a.Blood<=0) continue;
        if(a.NowState!=HUMAN_STATE_IDLE&&a.NowState!=HUMAN_STATE_WALKING) continue;
        int targetSN=-1;
        double minDist=1e9;
        for(tagArmy& enenmy:info.enemy_armies){
            double d=calDistance(a.DR,a.UR,enemy.DR,enemy.UR);
            if (d > 15 * BLOCKSIDELENGTH) continue;
            if (enemy.Sort == AT_CHARIOT_ARCHER || enemy.Sort == AT_COMPOSITE_BOWMAN || enemy.Sort == AT_STONE_THROWER){
                 targetSN=enemy.SN;
                 break;  
           }
        }
        if(targetSN==-1&&!info.enemy_armies.empty()){
            for(tagArmy& enemy:info.enemy_armies){
                if (d < 15 * BLOCKSIDELENGTH && d < minDist) {
                    minDist = d;
                    targetSN = enemy.SN;
                 }
            }
        }
        if(targetSN=-1&&stage>=stageAttack&&!info.enemy_armies.empty()){
            for(tagArmy& enemy:info.enemy_armies){
                double eDR = blockToDetail(enemy.BlockDR);
                double eUR = blockToDetail(enemy.BlockUR);
                double d = calDistance(a.DR, a.UR, eDR, eUR);
                if (d < 20 * BLOCKSIDELENGTH && d < minDist) {
                    minDist = d;
                    targetSN = enemy.SN;
                 }
            }
        }
        if(targetSN!=-1){
            HumanAction(a.SN,targetSN);
        }
    }
}


void UsrAI::processData()
{    tagInfo info = getInfo();
     if (info.GameFrame % 5 != 0) return;
     


}
