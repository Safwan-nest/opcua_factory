#include "hierarchy.h"

void build_factory_hierarchy(UA_Server *server) {
    // 1. Create Factory01 [O03]
    UA_NodeId factoryId;
    UA_ObjectAttributes factoryAttr = UA_ObjectAttributes_default;
    factoryAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Factory01");
    
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Factory01"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            factoryAttr, NULL, &factoryId);

    // 2. Create ProductionLine01 (Child of Factory01) [O04]
    UA_NodeId lineId;
    UA_ObjectAttributes lineAttr = UA_ObjectAttributes_default;
    lineAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ProductionLine01");
    
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1001),
                            factoryId, // Set Factory01 as the parent
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "ProductionLine01"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            lineAttr, NULL, &lineId);
                            
    // 3. Create FeedingStation (Child of ProductionLine01) [O05]
    UA_ObjectAttributes feedingAttr = UA_ObjectAttributes_default;
    feedingAttr.displayName = UA_LOCALIZEDTEXT("en-US", "FeedingStation");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1002),
                            lineId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "FeedingStation"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            feedingAttr, NULL, NULL);

    // 4. Create AssemblyStation (Child of ProductionLine01) [O06]
    UA_ObjectAttributes assemblyAttr = UA_ObjectAttributes_default;
    assemblyAttr.displayName = UA_LOCALIZEDTEXT("en-US", "AssemblyStation");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1003),
                            lineId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "AssemblyStation"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            assemblyAttr, NULL, NULL);

    // 5. Create InspectionStation (Child of ProductionLine01) [O07]
    UA_ObjectAttributes inspectAttr = UA_ObjectAttributes_default;
    inspectAttr.displayName = UA_LOCALIZEDTEXT("en-US", "InspectionStation");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1004),
                            lineId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "InspectionStation"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            inspectAttr, NULL, NULL);
}
