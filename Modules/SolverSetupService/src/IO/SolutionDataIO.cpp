#include <vtkIntArray.h>
#include <vtkDoubleArray.h>

#include <SolutionData.h>
#include "SolutionDataIO.h"

#include <IO/SolverSetupServiceIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>

#include <QDataStream>
#include <QFile>

REGISTER_IOUTILDATA_SERIALIZER(SolutionData, crimson::SolverSetupServiceIOMimeTypes::SOLUTIONDATA_DEFAULT_EXTENSION())

namespace crimson
{

SolutionDataIO::SolutionDataIO()
    : AbstractFileIO(SolutionData::GetStaticNameOfClass(), SolverSetupServiceIOMimeTypes::SOLUTIONDATA_MIMETYPE(),
                     "Solution data")
{
    RegisterService();
}

SolutionDataIO::~SolutionDataIO() {}

namespace detail
{
template <typename ArrayT>
vtkSmartPointer<vtkDataArray> make_vtk_array();

template <>
vtkSmartPointer<vtkDataArray> make_vtk_array<int>()
{
    return vtkIntArray::New();
}

template <>
vtkSmartPointer<vtkDataArray> make_vtk_array<double>()
{
    return vtkDoubleArray::New();
}

template <typename ArrayT, typename StreamT = ArrayT>
vtkSmartPointer<vtkDataArray> readDataArrayFromStream(QDataStream& stream, const QString& arrayName, qint64 nComponents,
    qint64 nTuples)
{
    auto dataArray = make_vtk_array<ArrayT>();

    dataArray->SetNumberOfComponents(gsl::narrow_cast<int>(nComponents));
    dataArray->SetNumberOfTuples(gsl::narrow_cast<int>(nTuples));
    dataArray->SetName(arrayName.toStdString().c_str());

    auto value = StreamT{0};

    for (int i = 0; i < dataArray->GetNumberOfComponents() * dataArray->GetNumberOfTuples(); ++i) {
        stream >> value;
        dataArray->SetVariantValue(i, gsl::narrow_cast<ArrayT>(value));
    }

    return dataArray;
}

template <typename ArrayT>
ArrayT get(vtkVariant variant);

template <>
int get<int>(vtkVariant variant)
{
    return variant.ToInt();
}

template <>
double get<double>(vtkVariant variant)
{
    return variant.ToDouble();
}

template <typename ArrayT, typename StreamT = ArrayT>
void writeDataArrayToStream(QDataStream& stream, vtkDataArray* dataArray)
{
    for (int i = 0; i < dataArray->GetNumberOfComponents() * dataArray->GetNumberOfTuples(); ++i) {
        stream << static_cast<StreamT>(get<ArrayT>(dataArray->GetVariantValue(i)));
    }
}
}

std::vector<itk::SmartPointer<mitk::BaseData>> SolutionDataIO::Read()
{
    QFile file{QString::fromStdString(GetLocalFileName())};
    file.open(QIODevice::ReadOnly);
    QDataStream inStream{&file};

    auto dataType = qint64{0};
    auto arrayName = QString{};
    auto nComponents = qint64{0};
    auto nTuples = qint64{0};

    inStream >> dataType >> arrayName >> nComponents >> nTuples;

    if (inStream.status() != QDataStream::Ok) {
        mitkThrow() << "Failed to read header data from file " << GetLocalFileName();
    }

    vtkSmartPointer<vtkDataArray> data;

    switch (dataType) {
    case VTK_DOUBLE:
        data = detail::readDataArrayFromStream<double>(inStream, arrayName, nComponents, nTuples);
        break;
    case VTK_INT:
        data = detail::readDataArrayFromStream<int, qint64>(inStream, arrayName, nComponents, nTuples);
        break;
    default:
        mitkThrow() << "Unknown data type " << dataType << " read from file " << GetLocalFileName();
    }

    if (inStream.status() != QDataStream::Ok) {
        mitkThrow() << "Failed to read array data from file " << GetLocalFileName();
    }

    return {itk::SmartPointer<mitk::BaseData>{SolutionData::New(data).GetPointer()}};
}

void SolutionDataIO::Write()
{
    auto solutionData = static_cast<const SolutionData*>(this->GetInput());

    if (!solutionData) {
        mitkThrow() << "Input solution data has not been set!";
    }

    QFile file{QString::fromStdString(GetOutputLocation())};
    file.open(QIODevice::WriteOnly);
    QDataStream outStream{&file};

    auto dataArray = solutionData->getArrayData();

    outStream << qint64{dataArray->GetDataType()};
    outStream << QString::fromLatin1(dataArray->GetName());
    outStream << qint64{dataArray->GetNumberOfComponents()};
    outStream << qint64{dataArray->GetNumberOfTuples()};

    switch (dataArray->GetDataType()) {
    case VTK_DOUBLE:
        detail::writeDataArrayToStream<double>(outStream, dataArray);
        break;
    case VTK_INT:
        detail::writeDataArrayToStream<int, qint64>(outStream, dataArray);
        break;
    default:
        mitkThrow() << "Unsupported solution data type";
    }
}
}