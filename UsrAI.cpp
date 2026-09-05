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
    static vector<int> resourcetask;
    static vector<int> tasktype;
    if((int)resourcetask.size()!=info.farmers.size()){
        resourcetask.assign(info.farmer.size(),-1);
        tasktype.assign(info.farmer.size(),-1);
    }
    for (auto i=0;i<info.farmers.size();i++) {
        tagFarmer& farmer=info.farmers[i];
        if (resourcetask[i] == -1) continue;
        if(farmer.Blood<=0||farmer.NowState==HUMAN_STATE_IDLE){
            resourcetask[i]=-1;
            tasktype[i]=-1;
            continue;
        }
        bool resourceExist=false;
        for(tagResource& r:info.resources){
            if(r.Type==RESOURCE_TYPE&&r.SN=resourcetask[i]&&r.Cnt>0){
                resourceExist=true;
                break;
            }
        }
        if(resourceExist==false){
            resourcetask[i]=-1;
            tasktype[i]=-1;
        }
    }
    int currentcount=0;
    for (auto i=0;i<info.farmers.size();i++){
        if(tasktype[i]==resourceType){currentcount++;}
    }
        if(currentcount>=num){return;}
        for (auto i=0;i<info.farmers.size();i++){
            tagFarmer& farmer=info.farmers[i];
            if (farmer.FarmerSort != FARMERTYPE_FARMER) continue;
            if (farmer.Blood <= 0) continue;
            if (farmer.NowState != HUMAN_STATE_IDLE) continue;
            if (resourcetask[i] != -1) continue;
            //寻找最近的资源
            int targetSN = -1;
            double minDist = 1e9;
            for (const tagResource& r : info.resources) {
                if (r.Type != resourceType||r.Cnt <= 0) continue;
                double d = calDistance(f.DR, f.UR, r.DR, r.UR);
                if (d < minDist) { 
                    minDist = d; 
                    targetSN = r.SN; 
                }
            }
            if(targetSN==-1) return;
            HumanAction(f.SN,targetSN);
            resourcetask[i]=targetSN;
            tasktype[i]=resourceType;
            currentCount++;
            if(currentCount>=targetCount) break;
        }
}
//打猎：羚羊
static void hunting(tagInfo& info){
    static vector<int> huntTasks; 
    if ((int)huntTasks.size()!= (int)info.farmers.size()) {
        huntTasks.assign(info.farmers.size(), -1);
    }
    for (auto i = 0; i < info.farmers.size(); i++) {
        tagFarmer& f = info.farmers[i];
        if (huntTasks[i] == -1) continue;
        if (f.Blood <= 0 || f.NowState == HUMAN_STATE_IDLE) {
            huntTasks[i] = -1;
            continue;
        }
        bool exists = false;
        for (tagResource& r : info.resources) {
            if (r.SN == huntTasks[i] && r.Type == RESOURCE_GAZELLE && 
                (r.Blood > 0 || r.Cnt > 0)) {
                exists = true;
                break;
            }
        }
        if (exists==false) huntTasks[i] = -1;
    }
    vector<tagResource*> gazelles;
    for (tagResource& r : info.resources) {
        if (r.Type == RESOURCE_GAZELLE && (r.Blood > 0 || r.Cnt > 0)) {
            gazelles.push_back(&r);
        }
    }
    if (gazelles.empty()) {
        for (auto i = 0; i < info.farmers.size(); i++) {
            huntTasks[i] = -1;
        }
        return;
    }
    int currentCount = 0;
    for (auto i = 0; i < info.farmers.size(); i++) {
        if (huntTasks[i] != -1) currentCount++;
    }
    int targetCount = 2;
    if (currentCount >= targetCount) return;
    tagResource* target = nullptr;
    vector<int> lockedSNs;
    for (auto i = 0; i < info.farmers.size(); i++) {
        if (huntTasks[i] != -1) {
            lockedSNs.push_back(huntTasks[i]);
        }
    }
    for (tagResource* g : gazelles) {
        bool isLocked = false;
        for (int sn : lockedSNs) {
            if (sn == g->SN) { isLocked = true; break; }
        }
        if (!isLocked) {
            target = g;
            break;
        }
    }
    if (target == nullptr && !gazelles.empty()) {
        target = gazelles[0];
    }
    if (target == nullptr) return;
    int assigned = 0;
    for (auto i = 0; i < info.farmers.size() && assigned < 2; i++) {
         tagFarmer& f = info.farmers[i];
        if (f.FarmerSort != FARMERTYPE_FARMER) continue;
        if (f.Blood <= 0) continue;
        if (f.NowState != HUMAN_STATE_IDLE) continue;
        if (huntTasks[i] != -1) continue
        HumanAction(f.SN, target->SN);
        huntTasks[i] = target->SN;
        assigned++;
    }
}
//建筑：市镇中心，谷仓，市场，兵营、靶场 、马厩
static void manageBuildings(const tagInfo& info) {
    // ============================================================
    // 第一步：收集信息（查找关键建筑SN，统计数量）
    // ============================================================
    // TODO: 遍历 info.buildings，找到各建筑的SN
    
    // ============================================================
    // 第二步：市镇中心（造村民 + 升级时代）
    // ============================================================
    // TODO: 生产村民 + 升级铜器
    
    // ============================================================
    // 第三步：谷仓（研发箭塔科技）
    // ============================================================
    // TODO: 研发 BUILDING_GRANARY_ARROWTOWER
    
    // ============================================================
    // 第四步：市场（研发科技）
    // ============================================================
    // TODO: 车轮 → 木材加工 → 驯养动物 → 金矿开采
    
    // ============================================================
    // 第五步：仓库（研发攻防科技）
    // ============================================================
    // TODO: 工具使用 → 步兵护甲
    
    // ============================================================
    // 第六步：兵营（训练士兵）
    // ============================================================
    // TODO: 棍棒兵 / 阔剑兵
    
    // ============================================================
    // 第七步：靶场（训练弓箭手）
    // ============================================================
    // TODO: 弓箭手
    
    // ============================================================
    // 第八步：马厩（训练骑兵）
    // ============================================================
    // TODO: 侦察骑兵 / 骑兵
    
    // ============================================================
    // 第九步：学院（训练方阵兵）
    // ============================================================
    // TODO: 方阵兵
}

void UsrAI::processData()
{    tagInfo info = getInfo();
     if (info.GameFrame % 5 != 0) return;



}
