#pragma once

#include <QAsyncTaskAdapter.h>
#include <VesselForestData.h>
#include <VesselPathAbstractData.h>
#include <AsyncTaskWithResult.h>
#include <ISolidModelKernel.h>
#include <CompositeTask.h>
#include <mitkDataNode.h>

#include <map>
#include <thread>

/*!
 *  \brief An asynchronous task which detects whether a pair of vessels intersects.
 *  The result type is the same as that of crimson::ISolidModelKernel::intersectionEdgeLength.
 */
class DetectSingleIntersectionTask : public crimson::async::TaskWithResult<std::tuple<double, bool, int>>
{
public:
    DetectSingleIntersectionTask(mitk::BaseData::Pointer solid1_, mitk::BaseData::Pointer solid2_)
        : solid1(solid1_)
        , solid2(solid2_)
    {
    }

    std::tuple<State, std::string> runTask() override
    {
        if (isCancelling()) {
            return std::make_tuple(crimson::async::Task::State_Cancelled, std::string("Operation cancelled."));
        }

        std::string names[2];
        solid1->GetPropertyList()->GetStringProperty("name", names[0]);
        solid2->GetPropertyList()->GetStringProperty("name", names[1]);

        MITK_INFO << "Testing for intersection between " << names[0] << " and " << names[1];
        setResult(crimson::ISolidModelKernel::intersectionEdgeLength(solid1, solid2));
        MITK_INFO << "Intersection between " << names[0] << " and " << names[1] << " "
                  << (std::get<0>(getResult().get()) > 0 ? "found" : "not found");
        return std::make_tuple(State_Finished, std::string());
    }

    // private:
    mitk::BaseData::Pointer solid1;
    mitk::BaseData::Pointer solid2;
};

/*! \brief   A composite task execution strategy for detecting intersections. 
 * 
 * It avoids using the same vessels in tasks running concurrently, which would slow down the computation
 * (seemingly due to a mutex within OpenCASCADE disallowing usage of the same shape in multiple threads).
 */
class DetectIntersectionsExecutionStrategy : public crimson::ICompositeExecutionStrategy
{
public:
    DetectIntersectionsExecutionStrategy(
        crimson::VesselForestData::Pointer vesselForest,
        const std::map<mitk::DataNode::Pointer, mitk::DataNode::Pointer>& vesselNodeToLoftModelNodeMap,
        bool performCleanDetection)
    {
        for (auto iter = vesselNodeToLoftModelNodeMap.begin(); iter != --vesselNodeToLoftModelNodeMap.end(); ++iter) {
            auto iter2 = iter;
            for (++iter2; iter2 != vesselNodeToLoftModelNodeMap.end(); ++iter2) {
                auto vessel1 = static_cast<crimson::VesselPathAbstractData*>(iter->first->GetData());
                auto vessel2 = static_cast<crimson::VesselPathAbstractData*>(iter2->first->GetData());
                if (!vesselForest->getVesselUsedInBlending(vessel1) || !vesselForest->getVesselUsedInBlending(vessel2)) {
                    continue;
                }
                iter->second->GetData()->GetPropertyList()->SetStringProperty("name", iter->first->GetName().c_str());
                iter2->second->GetData()->GetPropertyList()->SetStringProperty("name", iter2->first->GetName().c_str());

                auto bop =
                    vesselForest->findBooleanOperationInfo(std::make_pair(vessel1->getVesselUID(), vessel2->getVesselUID()))
                        .bop;
                if (bop == crimson::VesselForestData::bopInvalidLast || performCleanDetection) {
                    auto task =
                        std::make_shared<DetectSingleIntersectionTask>(iter->second->GetData(), iter2->second->GetData());
                    tasks.push_back(task);
                    taskVesselIndices[task.get()] =
                        std::make_pair(static_cast<int>(std::distance(vesselNodeToLoftModelNodeMap.begin(), iter)),
                                       static_cast<int>(std::distance(vesselNodeToLoftModelNodeMap.begin(), iter2)));
                }
            }
        }
    }

    const std::vector<std::shared_ptr<crimson::async::Task>>& allTasks() const override { return tasks; }

    std::vector<std::shared_ptr<crimson::async::Task>> startingTasks() override
    {
        std::vector<std::shared_ptr<crimson::async::Task>> startingTasksVector;

        int nStartingThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);

        for (int i = 0; i < nStartingThreads; ++i) {
            auto nextTask = tryFindNextTask();

            if (nextTask) {
                startingTasksVector.push_back(nextTask);
            } else {
                break;
            }
        }

        return startingTasksVector;
    }

    std::shared_ptr<crimson::async::Task> nextTask(const std::shared_ptr<crimson::async::Task>& finishedTask) override
    {
        currentlyUsedIndices.erase(taskVesselIndices[finishedTask.get()].first);
        currentlyUsedIndices.erase(taskVesselIndices[finishedTask.get()].second);
        taskVesselIndices.erase(finishedTask.get());

        return tryFindNextTask();
    }

private:
    std::shared_ptr<crimson::async::Task> tryFindNextTask()
    {
        for (const auto& taskVesselIndicesPair : taskVesselIndices) {
            if (currentlyUsedIndices.count(taskVesselIndicesPair.second.first) == 0 &&
                currentlyUsedIndices.count(taskVesselIndicesPair.second.second) == 0) {
                currentlyUsedIndices.insert(taskVesselIndicesPair.second.first);
                currentlyUsedIndices.insert(taskVesselIndicesPair.second.second);
                return *std::find_if(tasks.begin(), tasks.end(),
                                     [&taskVesselIndicesPair](const std::shared_ptr<crimson::async::Task>& t) {
                                         return t.get() == taskVesselIndicesPair.first;
                                     });
            }
        }

        return nullptr;
    }

    std::vector<std::shared_ptr<crimson::async::Task>> tasks;

    std::map<crimson::async::Task*, std::pair<int, int>> taskVesselIndices;
    std::set<int> currentlyUsedIndices;
};

/*! \brief   An async task that detects all the intersecting pairs of vessels in the vessel tree. */
class DetectIntersectionsTask : public crimson::QAsyncTaskAdapter
{
    Q_OBJECT
public:
    DetectIntersectionsTask(crimson::VesselForestData::Pointer vesselForest,
                            const std::map<mitk::DataNode::Pointer, mitk::DataNode::Pointer>& vesselNodeToLoftModelNodeMap,
                            bool performCleanDetection)
        : vesselForest(vesselForest)
        , vesselNodeToLoftModelNodeMap(vesselNodeToLoftModelNodeMap)
    {
        executionStrategy = std::static_pointer_cast<crimson::ICompositeExecutionStrategy>(
            std::make_shared<DetectIntersectionsExecutionStrategy>(vesselForest, vesselNodeToLoftModelNodeMap,
                                                                   performCleanDetection));
        setTask(std::make_shared<crimson::CompositeTask>(executionStrategy));
        connect(this, &DetectIntersectionsTask::taskStateChanged, this, &DetectIntersectionsTask::processTaskStateChange);
    }

private slots:
    void processTaskStateChange(crimson::async::Task::State state, QString /*message*/)
    {
        if (state == crimson::async::Task::State_Finished) {
            auto compositeTask = static_cast<crimson::CompositeTask*>(getTask().get());

            for (const std::shared_ptr<crimson::async::Task>& childTaskPtr : executionStrategy->allTasks()) {
                auto singleIntersectionTask = static_cast<DetectSingleIntersectionTask*>(childTaskPtr.get());

                // Get the vessel datas
                crimson::VesselPathAbstractData* vessel1 = nullptr;
                crimson::VesselPathAbstractData* vessel2 = nullptr;
                for (auto iter = vesselNodeToLoftModelNodeMap.begin(); iter != vesselNodeToLoftModelNodeMap.end(); ++iter) {
                    if (iter->second->GetData() == singleIntersectionTask->solid1.GetPointer()) {
                        vessel1 = static_cast<crimson::VesselPathAbstractData*>(iter->first->GetData());
                    } else if (iter->second->GetData() == singleIntersectionTask->solid2.GetPointer()) {
                        vessel2 = static_cast<crimson::VesselPathAbstractData*>(iter->first->GetData());
                    }
                }
                assert(vessel1 && vessel2);

                double intersectionEdgeLength;
                bool removesFace;
                int removedFaceOwnerIndex;
                std::tie(intersectionEdgeLength, removesFace, removedFaceOwnerIndex) = *(singleIntersectionTask->getResult());

                auto booleanOperation = crimson::VesselForestData::BooleanOperationInfo{};
                booleanOperation.vessels = std::make_pair(vessel1->getVesselUID(), vessel2->getVesselUID());
                booleanOperation.bop = crimson::VesselForestData::bopFuse;
                booleanOperation.removesFace = removesFace;
                if (removesFace) {
                    booleanOperation.removedFaceOwnerUID = removedFaceOwnerIndex == 0 ? vessel1->getVesselUID() : vessel2->getVesselUID();
                }

                if (intersectionEdgeLength >= 0) {
                    auto prevBop = vesselForest->findBooleanOperationInfo(booleanOperation.vessels);
                    // Update only internal information if bop already existed, maintain type and order of operands 
                    if (prevBop.bop != crimson::VesselForestData::bopInvalidLast) {
                        booleanOperation.bop = prevBop.bop;
                        booleanOperation.vessels = std::move(prevBop.vessels);
                    }

                    auto currentFilletSizeIter = vesselForest->getFilletSizeInfos().find(booleanOperation.vessels);

                    vesselForest->replaceBooleanOperationInfo(booleanOperation);
                    if (currentFilletSizeIter == vesselForest->getFilletSizeInfos().end()) {
                        // Clean new intersection - propose fillet size
                        vesselForest->setFilletSizeInfo(booleanOperation.vessels,
                                                        intersectionEdgeLength / (2 * vnl_math::pi) *
                                                            0.1); // Set fillet size to 10% of the approximate radius
                    }
                } else {
                    // Intersection no longer present - remove the information from the vessel tree
                    vesselForest->removeBooleanOperationInfo(booleanOperation.vessels);
                    vesselForest->removeFilletSizeInfo(booleanOperation.vessels);
                }
            }
        }
    }

private:
    crimson::VesselForestData::Pointer vesselForest;
    std::map<mitk::DataNode::Pointer, mitk::DataNode::Pointer> vesselNodeToLoftModelNodeMap;
    std::shared_ptr<crimson::ICompositeExecutionStrategy> executionStrategy;
};
