#include <array>

#include <mitkTestingMacros.h>
#include <mitkTestFixture.h>
#include <mitkIOUtil.h>

#include <VesselForestData.h>
#include <vtkParametricSplineVesselPathData.h>
#include <IO/VesselTreeIOMimeTypes.h>

bool operator==(const crimson::VesselForestData::BooleanOperationInfo& i1,
                const crimson::VesselForestData::BooleanOperationInfo& i2)
{
    return i1.vessels == i2.vessels && i1.bop == i2.bop && i1.removesFace == i2.removesFace && i1.removedFaceOwnerUID == i2.removedFaceOwnerUID;
}

std::ostream& operator<<(std::ostream& os, const crimson::VesselForestData::BooleanOperationInfo& ii)
{
    os << "(" << ii.vessels.first << " " << ii.vessels.second << ") " << ii.bop << " " << ii.removesFace;
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& c)
{
    for (auto& v : c) {
        os << v << " ";
    }
    return os;
}

class VesselTreeDataTestSuite : public mitk::TestFixture
{
    CPPUNIT_TEST_SUITE(VesselTreeDataTestSuite);

    MITK_TEST(testAddRemovePaths);
    MITK_TEST(testFilletSizeAndUseInBlending);
    MITK_TEST(testReadWrite);
    MITK_TEST(testConnectedComponents);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() { vesselTree = crimson::VesselForestData::New().GetPointer(); }

    void tearDown() { vesselTree = nullptr; }

    void testAddRemovePaths()
    {
        std::array<crimson::vtkParametricSplineVesselPathData::Pointer, 2> v = {
            crimson::vtkParametricSplineVesselPathData::New(), crimson::vtkParametricSplineVesselPathData::New()};

        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(0));

        vesselTree->insertVessel(v[0].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(1));

        // Test multiple insertions of the same vessel
        vesselTree->insertVessel(v[0].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(1));

        vesselTree->insertVessel(v[1].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(2));

        // Test iteration
        for (const crimson::VesselPathAbstractData::VesselPathUIDType& uid : vesselTree->getVessels()) {
            bool knownVesselFound = false;
            for (const auto& vessel : v) {
                if (uid == vessel->getVesselUID()) {
                    knownVesselFound = true;
                    break;
                }
            }
            CPPUNIT_ASSERT_MESSAGE("Vessel not found during iteration", knownVesselFound);
        }

        vesselTree->removeVessel(v[0].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(1));

        // Test removal of vessel not present in tree
        vesselTree->removeVessel(v[0].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(1));

        vesselTree->removeVessel(v[1].GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->getVessels().size(), static_cast<size_t>(0));
    }

    void testFilletSizeAndUseInBlending()
    {
        crimson::vtkParametricSplineVesselPathData::Pointer v1 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v2 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v3 = crimson::vtkParametricSplineVesselPathData::New();

        auto v1uid = v1->getVesselUID();
        auto v2uid = v2->getVesselUID();
        auto v3uid = v3->getVesselUID();

        vesselTree->insertVessel(v1.GetPointer());
        vesselTree->insertVessel(v2.GetPointer());
        vesselTree->insertVessel(v3.GetPointer());

        std::vector<crimson::VesselForestData::BooleanOperationInfo> bopInfos = {
            {{v1uid, v2uid}, crimson::VesselForestData::bopFuse, true, v2uid},
            {{v1uid, v3uid}, crimson::VesselForestData::bopCut, false, ""},
            {{v2uid, v3uid}, crimson::VesselForestData::bopCommon, true, v3uid},
        };

        // Non-existent pair
        CPPUNIT_ASSERT_EQUAL(vesselTree->findBooleanOperationInfo(bopInfos[0].vessels).bop,
                             crimson::VesselForestData::bopInvalidLast);

        // Arbitrary order of vessels check
        for (const auto& bopInfo : bopInfos) {
            vesselTree->addBooleanOperationInfo(bopInfo);
            CPPUNIT_ASSERT_EQUAL(vesselTree->findBooleanOperationInfo(bopInfo.vessels), bopInfo);
            CPPUNIT_ASSERT_EQUAL(
                vesselTree->findBooleanOperationInfo(std::make_pair(bopInfo.vessels.second, bopInfo.vessels.first)), bopInfo);
        }

        for (int i = 0; i < bopInfos.size(); ++i) {
            CPPUNIT_ASSERT_EQUAL(vesselTree->getBooleanOperations()[i], bopInfos[i]);
        }

        // Test swapping
        vesselTree->swapBooleanOperations(bopInfos.begin()->vessels, bopInfos.rbegin()->vessels);
        std::iter_swap(bopInfos.begin(), bopInfos.rbegin());
        for (int i = 0; i < bopInfos.size(); ++i) {
            CPPUNIT_ASSERT_EQUAL(vesselTree->getBooleanOperations()[i], bopInfos[i]);
        }

        // Fillet tests
        std::vector<double> filletSizes = {1.1, 7.3, 3.86};
        CPPUNIT_ASSERT(vesselTree->getFilletSizeInfos().size() == 0);
        for (int i = 0; i < filletSizes.size(); ++i) {
            vesselTree->setFilletSizeInfo(bopInfos[i].vessels, filletSizes[i]);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(vesselTree->getFilletSizeInfos().at(bopInfos[i].vessels), filletSizes[i], 1e-6);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                vesselTree->getFilletSizeInfos().at(std::make_pair(bopInfos[i].vessels.second, bopInfos[i].vessels.first)),
                filletSizes[i], 1e-6);
        }

        // Set fillet size through undoable operation test
        double newSize = 3.2;
        using FilletChangeOperation = crimson::VesselForestData::FilletChangeOperation;
        FilletChangeOperation doOp(FilletChangeOperation::operationChange, bopInfos[1].vessels, newSize);
        FilletChangeOperation undoOp(FilletChangeOperation::operationChange, bopInfos[1].vessels,
                                     vesselTree->getFilletSizeInfos().at(bopInfos[1].vessels));

        vesselTree->ExecuteOperation(&doOp);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(vesselTree->getFilletSizeInfos().at(bopInfos[1].vessels), newSize, 1e-6);
        vesselTree->ExecuteOperation(&undoOp);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(vesselTree->getFilletSizeInfos().at(bopInfos[1].vessels), filletSizes[1], 1e-6);

        // Use in blending flag
        CPPUNIT_ASSERT(vesselTree->getVesselUsedInBlending(v1));
        CPPUNIT_ASSERT(vesselTree->getVesselUsedInBlending(v2));
        CPPUNIT_ASSERT(vesselTree->getVesselUsedInBlending(v3));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveVessels()), static_cast<size_t>(3));

        vesselTree->setVesselUsedInBlending(v1, false);
        CPPUNIT_ASSERT(!vesselTree->getVesselUsedInBlending(v1));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveVessels()), static_cast<size_t>(2));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveBooleanOperations()), static_cast<size_t>(1));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveFilletSizeInfos()), static_cast<size_t>(1));

        vesselTree->setVesselUsedInBlending(v1, true);
        CPPUNIT_ASSERT(vesselTree->getVesselUsedInBlending(v1));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveVessels()), static_cast<size_t>(3));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveBooleanOperations()), static_cast<size_t>(3));
        CPPUNIT_ASSERT_EQUAL(boost::size(vesselTree->getActiveFilletSizeInfos()), static_cast<size_t>(3));

        // Removal of vessel should lead to no intersection reported
        vesselTree->removeVessel(v2.GetPointer());
        CPPUNIT_ASSERT_EQUAL(vesselTree->findBooleanOperationInfo(bopInfos[0].vessels).bop,
                             crimson::VesselForestData::bopInvalidLast);
        CPPUNIT_ASSERT_EQUAL(vesselTree->findBooleanOperationInfo(bopInfos[1].vessels), bopInfos[1]);
        CPPUNIT_ASSERT_EQUAL(vesselTree->findBooleanOperationInfo(bopInfos[2].vessels).bop,
                             crimson::VesselForestData::bopInvalidLast);
    }

    void testReadWrite()
    {
        // Prepare data
        crimson::vtkParametricSplineVesselPathData::Pointer v1 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v2 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v3 = crimson::vtkParametricSplineVesselPathData::New();

        vesselTree->insertVessel(v1.GetPointer());
        vesselTree->insertVessel(v2.GetPointer());
        vesselTree->insertVessel(v3.GetPointer());

        auto v1uid = v1->getVesselUID();
        auto v2uid = v2->getVesselUID();
        auto v3uid = v3->getVesselUID();

        std::vector<crimson::VesselForestData::BooleanOperationInfo> bopInfos = {
            {{v1uid, v2uid}, crimson::VesselForestData::bopFuse, true, v2uid},
            {{v1uid, v3uid}, crimson::VesselForestData::bopCut, false, ""},
            {{v2uid, v3uid}, crimson::VesselForestData::bopCommon, true, v3uid},
        };

        for (const auto& bopInfo : bopInfos) {
            vesselTree->addBooleanOperationInfo(bopInfo);
        }

        std::vector<double> filletSizes = {1.1, 7.3, 3.86};
        for (int i = 0; i < filletSizes.size(); ++i) {
            vesselTree->setFilletSizeInfo(bopInfos[i].vessels, filletSizes[i]);
        }

        vesselTree->setVesselUsedInBlending(v2, false);

        // Test read/write
        std::ofstream tmpStream;
        std::string filePath = mitk::IOUtil::CreateTemporaryFile(
            tmpStream,
            std::string("vesselTreeIOTest_XXXXXX." + crimson::VesselTreeIOMimeTypes::VESSELTREE_DEFAULT_EXTENSION()));
        tmpStream.close();

        CPPUNIT_ASSERT_NO_THROW(mitk::IOUtil::Save(vesselTree, filePath));

        crimson::VesselForestData::Pointer loadedVesselTree;
        CPPUNIT_ASSERT_NO_THROW(loadedVesselTree =
                                    dynamic_cast<crimson::VesselForestData*>(mitk::IOUtil::Load(filePath)[0].GetPointer()));
        CPPUNIT_ASSERT(loadedVesselTree != nullptr);

        // Compare saved and loaded vessel trees
        for (int i = 0; i < bopInfos.size(); ++i) {
            CPPUNIT_ASSERT_EQUAL(vesselTree->getBooleanOperations()[i], bopInfos[i]);
        }
        for (int i = 0; i < filletSizes.size(); ++i) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(vesselTree->getFilletSizeInfos().at(bopInfos[i].vessels), filletSizes[i], 1e-6);
        }

        std::remove(filePath.c_str());
    }

    void testConnectedComponents()
    {
        auto getNumberOfComponents = [](const crimson::VesselForestData::VesselPathUIDToConnectedComponentMap& c) {
            auto compIndices = std::set<int>{};
            for (const auto& vesselUidCompIndexPair : c) {
                compIndices.insert(vesselUidCompIndexPair.second);
            }
            return compIndices.size();
        };

        crimson::vtkParametricSplineVesselPathData::Pointer v1 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v2 = crimson::vtkParametricSplineVesselPathData::New();
        crimson::vtkParametricSplineVesselPathData::Pointer v3 = crimson::vtkParametricSplineVesselPathData::New();

        CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(0), vesselTree->computeActiveConnectedComponents().size());

        vesselTree->insertVessel(v1.GetPointer());
        vesselTree->insertVessel(v2.GetPointer());
        vesselTree->insertVessel(v3.GetPointer());

        auto v1uid = v1->getVesselUID();
        auto v2uid = v2->getVesselUID();
        auto v3uid = v3->getVesselUID();

        auto connectedComponents = vesselTree->computeActiveConnectedComponents();
        CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(3), getNumberOfComponents(connectedComponents));

        CPPUNIT_ASSERT(connectedComponents[v1uid] != connectedComponents[v2uid] &&
                       connectedComponents[v1uid] != connectedComponents[v3uid] &&
                       connectedComponents[v2uid] != connectedComponents[v3uid]);

        vesselTree->addBooleanOperationInfo(crimson::VesselForestData::BooleanOperationInfo{
            std::make_pair(v1uid, v2uid), crimson::VesselForestData::bopFuse, true, v2uid});

        connectedComponents = vesselTree->computeActiveConnectedComponents();
        CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(2), getNumberOfComponents(connectedComponents));
        CPPUNIT_ASSERT(connectedComponents[v1uid] == connectedComponents[v2uid] &&
                       connectedComponents[v1uid] != connectedComponents[v3uid]);

        vesselTree->setVesselUsedInBlending(v3uid, false);

        connectedComponents = vesselTree->computeActiveConnectedComponents();
        CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), getNumberOfComponents(connectedComponents));
        CPPUNIT_ASSERT(connectedComponents[v1uid] == connectedComponents[v2uid]);
        CPPUNIT_ASSERT(connectedComponents.find(v3uid) == connectedComponents.end());
    }

private:
    crimson::VesselForestData::Pointer vesselTree;
};

MITK_TEST_SUITE_REGISTRATION(VesselTreeData)
