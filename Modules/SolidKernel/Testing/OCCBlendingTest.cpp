#include <array>

#include "OCCTestingConfig.h"
#include <ISolidModelKernel.h>
#include <OCCBRepData.h>

#include <mitkTestingMacros.h>
#include <mitkTestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkIOUtil.h>
#include <mitkStandaloneDataStorage.h>
#include <mitkNodePredicateData.h>
#include <mitkNodePredicateDataType.h>

class OCCBlendingTestSuite : public mitk::TestFixture
{
    CPPUNIT_TEST_SUITE(OCCBlendingTestSuite);

    MITK_TEST(test);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {}

    void tearDown() {}

    void test()
    {

        std::string testDataPath{OCC_TEST_DATA_PATH};

        std::string mainPath = "PATH_2015041121151225218309";
        std::string firstBranch = "PATH_2015041121154450998782";
        std::string secondBranch = "PATH_2015041121521836260494";
        std::string thirdBranch = "PATH_2015121718270361012230";

        crimson::FaceIdentifier mainWall{std::set<std::string>{mainPath}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier mainInflow{std::set<std::string>{mainPath}, crimson::FaceIdentifier::ftCapInflow};
        crimson::FaceIdentifier mainOutflow{std::set<std::string>{mainPath}, crimson::FaceIdentifier::ftCapOutflow};

        crimson::FaceIdentifier firstBranchWall{std::set<std::string>{firstBranch}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier firstBranchOutflow{std::set<std::string>{firstBranch}, crimson::FaceIdentifier::ftCapOutflow};

        crimson::FaceIdentifier secondBranchWall{std::set<std::string>{secondBranch}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier secondBranchInflow{std::set<std::string>{secondBranch}, crimson::FaceIdentifier::ftCapInflow};
        crimson::FaceIdentifier secondBranchOutflow{std::set<std::string>{secondBranch}, crimson::FaceIdentifier::ftCapOutflow};

        crimson::FaceIdentifier thirdBranchWall{std::set<std::string>{thirdBranch}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier thirdBranchOutflow{std::set<std::string>{thirdBranch}, crimson::FaceIdentifier::ftCapOutflow};

        crimson::FaceIdentifier mainFirstBlend{std::set<std::string>{mainPath, firstBranch}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier mainSecondBlend{std::set<std::string>{mainPath, secondBranch}, crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier firstSecondBlend{std::set<std::string>{firstBranch, secondBranch},
                                                 crimson::FaceIdentifier::ftWall};
        crimson::FaceIdentifier secondThirdBlend{std::set<std::string>{secondBranch, thirdBranch},
                                                 crimson::FaceIdentifier::ftWall};

        crimson::FaceIdentifier threeWayBlend{std::set<std::string>{mainPath, firstBranch, secondBranch},
                                              crimson::FaceIdentifier::ftWall};

        MITK_TEST_OUTPUT(<< "Testing single vessel");
        testBlending(testDataPath + "/blending/single_vessel_blend.mitk",
            std::set<crimson::FaceIdentifier>{mainWall, mainInflow});


        MITK_TEST_OUTPUT(<< "Testing simple two way blend");
        testBlending(testDataPath + "/blending/two_way_blend.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, mainInflow, mainOutflow, firstBranchWall, firstBranchOutflow,
                                                       mainFirstBlend});

        MITK_TEST_OUTPUT(<< "Testing simple two way with fillets at ends");
        testBlending(testDataPath + "/blending/two_way_blend_fillet_at_ends.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, firstBranchWall, mainFirstBlend});

        MITK_TEST_OUTPUT(<< "Testing simple two way fuse (no filleting)");
        testBlending(testDataPath + "/blending/two_way_blend_no_fillet.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, mainInflow, mainOutflow, firstBranchWall, firstBranchOutflow});

        MITK_TEST_OUTPUT(<< "Testing simple two way blend plus a disconnected component with fillet at inflow");
        testBlending(testDataPath + "/blending/two_way_blend_disconnected_component.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, mainInflow, mainOutflow, firstBranchWall, firstBranchOutflow,
                                                       mainFirstBlend, secondBranchWall, secondBranchOutflow});

        MITK_TEST_OUTPUT(<< "Testing two disconnected two-way blends with fillet at one inflow");
        testBlending(testDataPath + "/blending/two_way_blend_disconnected_component_2.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, mainInflow, mainOutflow, firstBranchWall, firstBranchOutflow,
                                                       mainFirstBlend, secondBranchWall, secondBranchOutflow,
                                                       thirdBranchOutflow, thirdBranchWall, secondThirdBlend});

        MITK_TEST_OUTPUT(<< "Testing three way blend");
        testBlending(testDataPath + "/blending/three_way_blend.mitk",
                     std::set<crimson::FaceIdentifier>{mainWall, mainInflow, mainOutflow, firstBranchWall, firstBranchOutflow,
                                                       mainFirstBlend, secondBranchWall, secondBranchOutflow, mainSecondBlend,
                                                       firstSecondBlend, threeWayBlend});

        MITK_TEST_OUTPUT(<< "Testing custom boolean operations");
        testBlending(testDataPath + "/blending/custom_bops.mitk", std::set<crimson::FaceIdentifier>{});
    }

private:
    void testBlending(const std::string& fileName, const std::set<crimson::FaceIdentifier>& expectedFaceIds)
    {
        MITK_TEST_OUTPUT(<< "Sequential");
        testBlending(fileName, expectedFaceIds, false);
        MITK_TEST_OUTPUT(<< "Parallel");
        testBlending(fileName, expectedFaceIds, true);
    }

    void testBlending(const std::string& fileName, const std::set<crimson::FaceIdentifier>& expectedFaceIds, bool parallel)
    {
        auto storage = mitk::StandaloneDataStorage::New();
        mitk::IOUtil::Load(fileName, *storage.GetPointer());

        std::map<crimson::VesselForestData::VesselPathUIDType, mitk::BaseData::Pointer> brepDatas;

        // Find the path node
        mitk::DataNode* vesselTreeNode = storage->GetNode(mitk::TNodePredicateDataType<crimson::VesselForestData>::New());

        // Get the paths
        mitk::DataStorage::SetOfObjects::ConstPointer vesselPathNodes =
            storage->GetSubset(mitk::TNodePredicateDataType<crimson::VesselPathAbstractData>::New());

        for (const mitk::DataNode::Pointer& vesselNode : *vesselPathNodes) {
            mitk::DataNode* solidNode =
                storage->GetDerivations(vesselNode.GetPointer(), mitk::TNodePredicateDataType<crimson::OCCBRepData>::New())
                    ->front();
            brepDatas[static_cast<crimson::VesselPathAbstractData*>(vesselNode->GetData())->getVesselUID()] =
                solidNode->GetData();
        }

        auto vesselForest = static_cast<crimson::VesselForestData*>(vesselTreeNode->GetData());

        auto task = crimson::ISolidModelKernel::createBlendTask(brepDatas, vesselForest->getActiveBooleanOperations(),
                                                                vesselForest->getActiveFilletSizeInfos(), parallel);

        // Run task synchronously
        task->run();

        // Test successful completion
        CPPUNIT_ASSERT(task->getResult());

        // Test assignment of face identifiers
        crimson::OCCBRepData* resultingSolid = dynamic_cast<crimson::OCCBRepData*>(task->getResult()->GetPointer());
        CPPUNIT_ASSERT(resultingSolid != nullptr);

        if (expectedFaceIds.empty()) {
            return;
        }

        std::set<crimson::FaceIdentifier> remainingFaceIds{expectedFaceIds};

        for (int i = 0; i < resultingSolid->getFaceIdentifierMap().getNumberOfFaceIdentifiers(); ++i) {
            auto iter = remainingFaceIds.find(resultingSolid->getFaceIdentifierMap().getFaceIdentifier(i));
            CPPUNIT_ASSERT(iter != remainingFaceIds.end());
            remainingFaceIds.erase(iter);
        }

        CPPUNIT_ASSERT(remainingFaceIds.empty());
    }
};

MITK_TEST_SUITE_REGISTRATION(OCCBlending)
