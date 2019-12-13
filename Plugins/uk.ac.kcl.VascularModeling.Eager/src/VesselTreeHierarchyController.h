#pragma once

#include <mitkDataStorage.h>

#include "uk_ac_kcl_VascularModeling_Eager_Export.h"

namespace crimson {

/*!
 * \brief  A singleton class that tracks addition and deletion of data nodes to
 *  maintain the consistensy of a VesselForestData. 
 */
class VASCULARMODELINGEAGER_EXPORT VesselTreeHierarchyController : public itk::Object {
public:
    ///@} 
    /*! Singleton interface */
    static bool init();
    static VesselTreeHierarchyController* getInstance();
    static void term();
    ///@} 

    mitk::DataStorage::Pointer getDataStorage();

private:
    VesselTreeHierarchyController();
    ~VesselTreeHierarchyController();

    VesselTreeHierarchyController(const VesselTreeHierarchyController&) = delete;
    VesselTreeHierarchyController& operator=(const VesselTreeHierarchyController&) = delete;

    void _connectDataStorageEvents();
    void _disconnectDataStorageEvents();

    void nodeAdded(const mitk::DataNode*);
    void nodeRemoved(const mitk::DataNode*);

private:
    static VesselTreeHierarchyController* _instance;

    struct VesselTreeHierarchyControllerImpl;

    std::unique_ptr<VesselTreeHierarchyControllerImpl> _impl;
};

} // namespace crimson