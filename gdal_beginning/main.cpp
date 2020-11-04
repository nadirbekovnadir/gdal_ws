#include <iostream>
#include <filesystem>
#include <vector>

#include <gdal/cpl_conv.h>
#include <gdal_priv.h>

using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace fs = std::filesystem;

string GetFullName(string fileName, string path);

void PrintGDALDatasetInformation(GDALDataset* ds);
void PrintGDALRasterBandInformation(GDALRasterBand* rb);

void ReadRasterData(GDALRasterBand* rb, float* psl);

int main(int argc, char *argv[])
{
    string filesPath = "/home/NIR/Data/tiles";

    vector<string> filesFullNames;

    for (const auto entry : fs::directory_iterator(filesPath.c_str()))
        filesFullNames.push_back(entry.path());

    GDALAllRegister();

    for (int i = 0; i < filesFullNames.size(); i++)
    {
        cout << "File fullname: " << filesFullNames[i] << endl;

        GDALDataset* poDataset = nullptr;
        poDataset = (GDALDataset *)GDALOpen(filesFullNames[i].c_str(), GA_ReadOnly);
        PrintGDALDatasetInformation(poDataset);

        GDALRasterBand* poBand = nullptr;

        for (int i = 1; i <= poDataset->GetRasterCount(); i++)
        {
            poBand = poDataset->GetRasterBand(i);
            PrintGDALRasterBandInformation(poBand);
        }

        cout << endl;

        // float* pafScansine = nullptr;
        // ReadRasterData(poBand, pafScansine);
        // CPLFree(pafScansine);

        GDALClose(poDataset);
    }

    std::cout << "Program ended!" << std::endl;
    return 0;
}

string GetFullName(string fileName, string path)
{
    fileName = path + "/" + fileName;
    return fileName;
}

void PrintGDALDatasetInformation(GDALDataset* ds)
{
    double adfGeoTransform[6];

    cout << "Driver:\n"
         << "\t" << ds->GetDriver()->GetDescription() << "\n"
         << "\t" << ds->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME) << "\n";

    //cout << endl;

    cout << "Size:\n"
         << "\tRaster X: " << ds->GetRasterXSize() << "\n"
         << "\tRaster Y: " << ds->GetRasterYSize() << "\n"
         << "\tRaster count: " << ds->GetRasterCount() << "\n";

    //cout << endl;

    if (ds->GetProjectionRef() != nullptr)
        cout << "Projection is " << ds->GetProjectionRef() << endl;
    else 
        cout << "No projection set" << endl;

    //cout << endl;

    if (ds->GetGeoTransform(adfGeoTransform) == CE_None)
    {
        cout << "Geo transform:\n"
             << "\tOrigin: " << adfGeoTransform[0] << ", " << adfGeoTransform[3] << "\n"
             << "\tPixel size: " << adfGeoTransform[1] << ", " << adfGeoTransform[5] << "\n";
    }
    else
        cout << "Couldn't get geo transform" << "\n";

    char** md = ds->GetMetadata(nullptr);

    if (md != nullptr)
    {
        cout << "Metadata:" << endl;
        for (char** pair = md; pair != nullptr; pair++)
        {
            cout << "\t" << *pair << endl;
        }
    }
    else
        cout << "Metadata is empty" << endl;
    

    //cout << endl;
}

void PrintGDALRasterBandInformation(GDALRasterBand* rb)
{
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    double adfMinMax[2];

    rb->GetBlockSize(&nBlockXSize, &nBlockYSize);

    cout << "Block: " << nBlockXSize << ", " << nBlockYSize << "\n";
    cout << "Type: " << GDALGetDataTypeName(rb->GetRasterDataType()) << "\n";
    cout << "Color interp: " << GDALGetColorInterpretationName(rb->GetColorInterpretation()) << "\n";

    adfMinMax[0] = rb->GetMinimum(&bGotMin);
    adfMinMax[1] = rb->GetMinimum(&bGotMax);

    if (!(bGotMin && bGotMax))
        GDALComputeRasterMinMax(rb, TRUE, adfMinMax);

    cout << "Min: " << adfMinMax[0] << "\n";
    cout << "Max: " << adfMinMax[1] << "\n";

    cout << "Overviews count: " << rb->GetOverviewCount() << "\n";

    if (rb->GetColorTable() != nullptr)
        cout << "Color table entries count: " << rb->GetColorTable()->GetColorEntryCount() << "\n";
    else
        cout << "Color table not found" << "\n";

    //cout << endl;
}

void ReadRasterData(GDALRasterBand* rb, float* psl)
{
    int nXSize = rb->GetXSize();
    psl = (float*)CPLMalloc(sizeof(float) * nXSize);
    auto _ = rb->RasterIO(GF_Read, 0, 0, nXSize, 1, psl, nXSize, 1, GDT_Float32, 0, 0);
}