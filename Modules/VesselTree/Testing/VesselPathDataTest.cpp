#include <array>

#include <mitkTestingMacros.h>
#include <mitkTestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkInteractionConst.h>
#include <mitkPoint.h>
#include <mitkIOUtil.h>

#include <vtkParametricSplineVesselPathData.h>
#include <IO/VesselTreeIOMimeTypes.h>
#include <VesselPathOperation.h>

#include <chrono>
#include <random>

template<class VesselPathType, const std::string& extension>
class VesselPathAbstractDataTestSuite : public mitk::TestFixture {
    CPPUNIT_TEST_SUITE(VesselPathAbstractDataTestSuite);

    MITK_TEST(testAddSetRemovePoints);
    MITK_TEST(testBoundingBox);
    MITK_TEST(testPositionTangentNormal);
    MITK_TEST(testClosestPointRequests);
    MITK_TEST(testUndoRedo);
//     MITK_TEST(testPerformance);
    MITK_TEST(testReadWrite);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
        vesselPath = VesselPathType::New().GetPointer();

        points.resize(5);
        mitk::FillVector3D(points[0], 1.34, 2.701, 3.54);
        mitk::FillVector3D(points[1], -1.18, 3.236, -2.665);
        mitk::FillVector3D(points[2], 1e-3, -2.4, -6);
        mitk::FillVector3D(points[3], 0.5, 0.2, 1.1);
        mitk::FillVector3D(points[4], 1.6, 2.2, -4.3);
    }

    void tearDown()
    {
        vesselPath = nullptr;
    }

    void testAddSetRemovePoints()
    {
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)0);

        CPPUNIT_ASSERT_EQUAL(vesselPath->addControlPoint(points[0]), (crimson::VesselPathAbstractData::IdType)0);
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)1);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getParametricLength(), 0, 1e-6));
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(0), points[0], 1e-6));

        CPPUNIT_ASSERT_EQUAL(vesselPath->addControlPoint(points[1]), (crimson::VesselPathAbstractData::IdType)1);
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)2);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getParametricLength(), points[0].EuclideanDistanceTo(points[1]), 1e-6));
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(1), points[1], 1e-6));
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPointParameterValue(1), vesselPath->getParametricLength(), 1e-6));

        CPPUNIT_ASSERT_EQUAL(vesselPath->addControlPoint(1, points[2]), (crimson::VesselPathAbstractData::IdType)1);
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)3);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(1), points[2], 1e-6));

        CPPUNIT_ASSERT_EQUAL(vesselPath->addControlPoint(100, points[3]), (crimson::VesselPathAbstractData::IdType)3);
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)4);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(3), points[3], 1e-6));

        vesselPath->setControlPoint(2, points[4]);
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)4);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(2), points[4], 1e-6));

        CPPUNIT_ASSERT(vesselPath->removeControlPoint(1));
        CPPUNIT_ASSERT_EQUAL(vesselPath->controlPointsCount(), (crimson::VesselPathAbstractData::IdType)3);
        CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(1), points[4], 1e-6));
    }

    void testBoundingBox()
    {
        mitk::BoundingBox::Pointer bbox = mitk::BoundingBox::New();

        for (const mitk::Point3D& p : points) {
            vesselPath->addControlPoint(p);
            bbox->ConsiderPoint(p);
            CPPUNIT_ASSERT(mitk::Equal(*bbox.GetPointer(), *vesselPath->GetGeometry()->GetBoundingBox(), 1e-6, true));
        }
    }

    void testPositionTangentNormal()
    {
        vesselPath->setControlPoints(points);

        // Test if parametric values of control points are correct
        for (crimson::VesselPathAbstractData::IdType i = 0; i < vesselPath->controlPointsCount(); ++i)  {
            CPPUNIT_ASSERT(mitk::Equal(vesselPath->getPosition(vesselPath->getControlPointParameterValue(i)), vesselPath->getControlPoint(i), 1e-6, true));
        }

        // Test if normal and tangent are perpendicular
        int nTestPoints = 50;
        for (int i = 0; i <= nTestPoints; ++i) {
            crimson::VesselPathAbstractData::ParameterType p = (vesselPath->getParametricLength() * i) / nTestPoints;
            CPPUNIT_ASSERT(mitk::Equal(vesselPath->getTangentVector(p) * vesselPath->getNormalVector(p), 0.0, 1e-6, true));
        }
    }

    void testClosestPointRequests()
    {
    }

    void testUndoRedo()
    {
        // Test insert point
        mitk::Point3D newPoint;
        mitk::FillVector3D(newPoint, 3.14, 15.9, 2.65);
        int index = 2;

        crimson::VesselPathAbstractData::Pointer referencePath = VesselPathType::New().GetPointer();
        vesselPath->setControlPoints(points);
        referencePath->setControlPoints(points);

        CPPUNIT_ASSERT(vesselPathsEqual(vesselPath, referencePath, 1e-6, true));

        // Insert/undo insert point
        {
            auto doOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpINSERT, newPoint, index);
            auto undoOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpREMOVE, newPoint, index);
            vesselPath->ExecuteOperation(doOp.get());
            CPPUNIT_ASSERT_EQUAL(referencePath->controlPointsCount() + 1, vesselPath->controlPointsCount());
            CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(index), newPoint, 1e-6, true));
            vesselPath->ExecuteOperation(undoOp.get());
            CPPUNIT_ASSERT(vesselPathsEqual(vesselPath, referencePath, 1e-6, true));
        }

        // Remove/undo remove point
        {
            auto doOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpREMOVE, vesselPath->getControlPoint(index), index);
            auto undoOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpINSERT, vesselPath->getControlPoint(index), index);
            vesselPath->ExecuteOperation(doOp.get());
            CPPUNIT_ASSERT_EQUAL(referencePath->controlPointsCount() - 1, vesselPath->controlPointsCount());
            CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(index), referencePath->getControlPoint(index + 1), 1e-6, true));
            vesselPath->ExecuteOperation(undoOp.get());
            CPPUNIT_ASSERT(vesselPathsEqual(vesselPath, referencePath, 1e-6, true));
        }

        // Move point
        {
            auto doOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpMOVE, newPoint, index);
            auto undoOp = std::make_shared<crimson::VesselPathOperation>(mitk::OpMOVE, vesselPath->getControlPoint(index), index);
            vesselPath->ExecuteOperation(doOp.get());
            CPPUNIT_ASSERT_EQUAL(referencePath->controlPointsCount(), vesselPath->controlPointsCount());
            CPPUNIT_ASSERT(mitk::Equal(vesselPath->getControlPoint(index), newPoint, 1e-6, true));
            vesselPath->ExecuteOperation(undoOp.get());
            CPPUNIT_ASSERT(vesselPathsEqual(vesselPath, referencePath, 1e-6, true));
        }
    }

    void testPerformance()
    {
        int nControlPoints = 30;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> distribution(-100, 100);

        std::vector<mitk::Point3D> points;
        for (int i = 0; i < nControlPoints; ++i) {
            mitk::Point3D p;
            mitk::FillVector3D(p, distribution(gen), distribution(gen), distribution(gen));
            points.push_back(p);
        }
        vesselPath->setControlPoints(points);


        {
            int nIterations = 100;
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < nIterations; ++i) {
                vesselPath->setControlPoint(0, vesselPath->getControlPoint(0));
                vesselPath->getParametricLength();
            }
            auto end = std::chrono::high_resolution_clock::now();

            MITK_TEST_OUTPUT(<< "\nclean getParametricLength() " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / (double)nIterations << "ms");
        }
        
        {
            int nIterations = 20;
            int nClosestPointRequests = 30;

            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < nIterations; ++i) {
                vesselPath->setControlPoint(0, vesselPath->getControlPoint(0));

                for (int j = 0; j < nClosestPointRequests; ++j) {
                    mitk::Point3D p;
                    mitk::FillVector3D(p, distribution(gen), distribution(gen), distribution(gen));
                    vesselPath->getClosestPoint(p);
                }
            }
            auto end = std::chrono::high_resolution_clock::now();

            MITK_TEST_OUTPUT(<< "\ngetClosestPoint() " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / (double)nIterations / (double)nClosestPointRequests << "ms");
        }

        {
            int nIterations = 100;
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < nIterations; ++i) {
                vesselPath->setControlPoint(0, vesselPath->getControlPoint(0));
                vesselPath->getTangentVector(0.99 * vesselPath->getParametricLength());
            }
            auto end = std::chrono::high_resolution_clock::now();

            MITK_TEST_OUTPUT(<< "\nclean getTangentVector() " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / (double)nIterations << "ms");
        }

        {
            int nIterations = 10;
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < nIterations; ++i) {
                vesselPath->setControlPoint(0, vesselPath->getControlPoint(0));
                vesselPath->getNormalVector(0.99 * vesselPath->getParametricLength());
            }
            auto end = std::chrono::high_resolution_clock::now();

            MITK_TEST_OUTPUT(<< "\nclean getNormalVector() " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / (double)nIterations << "ms");
        }

    }

    void testReadWrite()
    {
        vesselPath->setControlPoints(points);

        std::ofstream tmpStream;
        std::string filePath = mitk::IOUtil::CreateTemporaryFile(tmpStream, std::string("vesselPathIOTest_XXXXXX." + extension));
        tmpStream.close();

        CPPUNIT_ASSERT_NO_THROW(mitk::IOUtil::Save(vesselPath, filePath));

        typename VesselPathType::Pointer loadedVesselPath;
        CPPUNIT_ASSERT_NO_THROW(loadedVesselPath = dynamic_cast<VesselPathType*>(mitk::IOUtil::Load(filePath)[0].GetPointer()));
        CPPUNIT_ASSERT(loadedVesselPath != nullptr);
        CPPUNIT_ASSERT(vesselPathsEqual(loadedVesselPath.GetPointer(), vesselPath, 1e-6, true));

        std::remove(filePath.c_str());
    }


private:
    bool vesselPathsEqual(const crimson::VesselPathAbstractData::Pointer& p1, const crimson::VesselPathAbstractData::Pointer& p2, double eps = mitk::eps, bool verbose = false)
    {
        if (p1->controlPointsCount() != p2->controlPointsCount()) {
            if (verbose) {
                MITK_TEST_OUTPUT(<< "Paths differ: control points count differ " << p1->controlPointsCount() << " != " << p2->controlPointsCount());
            }
            return false;
        }

        for (crimson::VesselPathAbstractData::IdType i = 0; i < p1->controlPointsCount(); ++i) {
            if (!mitk::Equal(p1->getControlPoint(i), p2->getControlPoint(i), eps, verbose)) {
                if (verbose) {
                    MITK_TEST_OUTPUT(<< "Paths differ: control point differs " << i);
                }

                return false;
            }
        }
        return true;
    }


    crimson::VesselPathAbstractData::Pointer vesselPath;
    std::vector<mitk::Point3D> points;
};

std::string vtkParametricSplineVesselPathExtension = crimson::VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION();

int VesselPathDataTest(int /*argc*/, char* /*argv*/[]) 
{
    CppUnit::TextUi::TestRunner runner;

    runner.addTest(VesselPathAbstractDataTestSuite<crimson::vtkParametricSplineVesselPathData, vtkParametricSplineVesselPathExtension>::suite());
    // runner.addTest(VesselPathAbstractDataTestSuite<SomeOtherVesselPathImplementation>::suite());

    return runner.run() ? 0 : 1;
}
