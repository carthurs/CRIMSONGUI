#include <array>

#include "OCCTestingConfig.h"
#include <ISolidModelKernel.h>
#include <OCCBRepData.h>

#include <mitkTestingMacros.h>
#include <mitkTestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkIOUtil.h>
#include <mitkStandaloneDataStorage.h>
#include <mitkNodePredicateDataType.h>

class OCCLoftingTestSuite : public mitk::TestFixture
{
    CPPUNIT_TEST_SUITE(OCCLoftingTestSuite);

    MITK_TEST(test);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {}

    void tearDown() {}

    void test()
    {
        std::string testDataPath{OCC_TEST_DATA_PATH};

        MITK_TEST_OUTPUT(<< "Testing simple cylinder lofting");
        testLofting(testDataPath + "/lofting/simple_cylinder.mitk");

        MITK_TEST_OUTPUT(<< "Testing use of inflow/outflow");
        testLofting(testDataPath + "/lofting/cylinder_use_inflow_outflow_as_wall.mitk",
                    std::set<crimson::FaceIdentifier::FaceType>{crimson::FaceIdentifier::ftWall});

        MITK_TEST_OUTPUT(<< "Testing complex loft");
        testLofting(testDataPath + "/lofting/complex_loft_aorta.mitk");

        MITK_TEST_OUTPUT(<< "Testing complex sweep");
        testLofting(testDataPath + "/lofting/complex_sweep_aorta.mitk");
    }

private:
    void testLofting(const std::string& fileName, const std::set<crimson::FaceIdentifier::FaceType>& expectedFaceTypes = {
                                                      crimson::FaceIdentifier::ftCapInflow,
                                                      crimson::FaceIdentifier::ftCapOutflow, crimson::FaceIdentifier::ftWall})
    {
        auto storage = mitk::StandaloneDataStorage::New();
        mitk::IOUtil::Load(fileName, *storage.GetPointer());

        // Find the path node
        mitk::DataNode* vesselNode = storage->GetNode(mitk::TNodePredicateDataType<crimson::VesselPathAbstractData>::New());

        // Get the contour nodes
        mitk::DataStorage::SetOfObjects::ConstPointer contourNodes =
            storage->GetDerivations(vesselNode, mitk::TNodePredicateDataType<mitk::PlanarFigure>::New());

        // Sort contours by parameter values
        std::vector<mitk::DataNode*> sortedContourNodes;
        sortedContourNodes.resize(contourNodes->size());
        std::transform(contourNodes->begin(), contourNodes->end(), sortedContourNodes.begin(),
                       [](mitk::DataNode::Pointer n) { return n.GetPointer(); });

        std::sort(sortedContourNodes.begin(), sortedContourNodes.end(), [](const mitk::DataNode* l, const mitk::DataNode* r) {
            float lv, rv;
            l->GetFloatProperty("lofting.parameterValue", lv);
            r->GetFloatProperty("lofting.parameterValue", rv);
            return lv < rv;
        });

        crimson::ISolidModelKernel::ContourSet contourSet(sortedContourNodes.size());
        std::transform(sortedContourNodes.begin(), sortedContourNodes.end(), contourSet.begin(), [](const mitk::DataNode* n) {
            auto pf = static_cast<mitk::PlanarFigure*>(n->GetData());
            float p;
            n->GetFloatProperty("lofting.parameterValue", p);
            pf->GetPropertyList()->SetFloatProperty("lofting.parameterValue", p);
            return pf;
        });

        bool useInflowAsWall = false, useOutflowAsWall = false;
        vesselNode->GetBoolProperty("lofting.useInflowAsWall", useInflowAsWall);
        vesselNode->GetBoolProperty("lofting.useOutflowAsWall", useOutflowAsWall);

        int loftingAlgorithm = crimson::ISolidModelKernel::laAppSurf;
        vesselNode->GetIntProperty("lofting.loftingAlgorithm", loftingAlgorithm);

        auto vesselPath = static_cast<crimson::VesselPathAbstractData*>(vesselNode->GetData());
        std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> task = crimson::ISolidModelKernel::createLoftTask(
            vesselPath, contourSet, useInflowAsWall, useOutflowAsWall, crimson::ISolidModelKernel::LoftingAlgorithm(loftingAlgorithm), 0.0, false, 1);

        // Run task synchronously
        task->run();

        // Test successful completion
        CPPUNIT_ASSERT(task->getResult());

        // Test assignment of face identifiers
        crimson::OCCBRepData* resultingSolid = dynamic_cast<crimson::OCCBRepData*>(task->getResult()->GetPointer());
        CPPUNIT_ASSERT(resultingSolid != nullptr);

        std::set<crimson::FaceIdentifier::FaceType> remainingFaceTypes{expectedFaceTypes};

        for (int i = 0; i < resultingSolid->getFaceIdentifierMap().getNumberOfFaceIdentifiers(); ++i) {
            CPPUNIT_ASSERT_EQUAL(resultingSolid->getFaceIdentifierMap().getFaceIdentifier(i).parentSolidIndices.size(),
                                 (size_t)1);
            CPPUNIT_ASSERT_EQUAL(vesselPath->getVesselUID(),
                                 *resultingSolid->getFaceIdentifierMap().getFaceIdentifier(i).parentSolidIndices.begin());

            auto iter = remainingFaceTypes.find(resultingSolid->getFaceIdentifierMap().getFaceIdentifier(i).faceType);
            CPPUNIT_ASSERT(iter != remainingFaceTypes.end());
            remainingFaceTypes.erase(iter);
        }

        CPPUNIT_ASSERT(remainingFaceTypes.empty());
    }
};

MITK_TEST_SUITE_REGISTRATION(OCCLofting)
