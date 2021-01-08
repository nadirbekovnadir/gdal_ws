#include <iostream>
#include <filesystem>
#include <vector>

#include "gdal/gdalwarper.h"
#include "gdal/cpl_conv.h"
#include "ogr_spatialref.h"
#include "gdal/gdal_priv.h"

using namespace std;
namespace fs = std::filesystem;

void ImagePreparation(
    const string& srcName, string dstPath);

void SimpleReprojection(
    const string& srcName, string dstPath);

void ReprojectionWithCreation(
    const string& srcName, string dstPath);

void ReprojectionWithCreation2(
    const string& srcName, string dstPath);

void ParseSrcFileName(
    const string& name, double& x, double& y);


const int UTMZone = 37;


int main(int, char**) 
{
    string origPath = "/home/NIR/Data/tiles";
    string srcPath = "/home/NIR/Data/tiles_changed";
    string dstPath = "/home/NIR/Data/Reprojection_results";

    vector<string> origNames;
    for (const auto entry : fs::directory_iterator(origPath.c_str()))
        origNames.push_back(entry.path());


    vector<string> dstNames;
    for (const auto entry : fs::directory_iterator(dstPath.c_str()))
        dstNames.push_back(entry.path());


    GDALAllRegister();    

    for (auto origName : origNames)
        ImagePreparation(origName, srcPath);

    vector<string> srcNames;
    for (const auto entry : fs::directory_iterator(srcPath.c_str()))
        srcNames.push_back(entry.path());

    for (auto srcName : srcNames)
        ReprojectionWithCreation(srcName, dstPath);
        //SimpleReprojection(srcNames[0], dstPath);
}



void ImagePreparation(
    const string& srcName, string dstPath)
{
    GDALDriverH hDriver = nullptr;
    GDALDataType eDT;

    GDALDataset* srcDS = nullptr;
    srcDS = GDALDataset::Open(srcName.c_str(), GA_ReadOnly);
    
    if (srcDS == nullptr)
    {
        cout << "ImagePreparation: srcDs is null" << endl;
        return;
    }

    const char *pszFormat = "GTiff";
    GDALDriver *poDriver = nullptr;
    char **papszMetadata = nullptr;

    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if(poDriver == NULL)
        exit(1);

    papszMetadata = poDriver->GetMetadata();
    if(CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        printf("Driver %s supports Create() method.\n", pszFormat);
        
    if(CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE) )
        printf("Driver %s supports CreateCopy() method.\n", pszFormat);
    else
    {
        cout 
            << "Driver " << pszFormat 
            << " not support CreateCopy()" << endl;

        return;
    }

    //GDALDestroyDriver(poDriver);
    
    string srcFileName = srcName.substr(srcName.find_last_of('/'));
    dstPath += srcFileName;

    GDALDataset* dstDS = nullptr;
    dstDS = poDriver->CreateCopy(dstPath.c_str(), srcDS, 
        FALSE, NULL, NULL, NULL);

    GDALClose(srcDS);


    OGRSpatialReference oGeoS;

    double moscowLot = 37.6155600;
    double satHeight = 2000;

    //oGeoS.SetProjCS("GEOS / WGS84");
    oGeoS.SetWellKnownGeogCS("WGS84");
    //oGeoS.SetGEOS(moscowLot, satHeight, 0, 0);

    char* dstWKT;
    oGeoS.exportToWkt(&dstWKT);
    cout << dstWKT;

    dstDS->SetProjection(dstWKT);
    CPLFree(dstWKT);


    GDALClose(dstDS);
}



void SimpleReprojection(
    const string& srcName, string dstPath)
{
    GDALDatasetH  hSrcDS, hDstDS;

    // Open input and output files.
    int pos = srcName.find_last_of("/");
    dstPath += srcName.substr(pos);

    GDALAllRegister();
    hSrcDS = GDALOpen( srcName.c_str(), GA_ReadOnly );
    hDstDS = GDALCreateCopy(
        GDALGetDriverByName("GTiff") , 
        dstPath.c_str(), hSrcDS, FALSE, 
        NULL, NULL, NULL);

    OGRSpatialReference oSRS;
    char *pszDstWKT;
    oSRS.SetUTM(11, TRUE);
    oSRS.SetWellKnownGeogCS("WGS84");
    oSRS.exportToWkt(&pszDstWKT);

    // cout << GDALGetProjectionRef(hDstDS) << endl << endl;
    GDALSetProjection(hDstDS, pszDstWKT);

    // Setup warp options.
    GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();
    psWarpOptions->hSrcDS = hSrcDS;
    psWarpOptions->hDstDS = hDstDS;
    psWarpOptions->nBandCount = 1;
    psWarpOptions->panSrcBands =
        (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
    psWarpOptions->panSrcBands[0] = 1;
    psWarpOptions->panDstBands =
        (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
    psWarpOptions->panDstBands[0] = 1;
    psWarpOptions->pfnProgress = GDALTermProgress;

    // Establish reprojection transformer.
    psWarpOptions->pTransformerArg =
        GDALCreateGenImgProjTransformer(hSrcDS,
                                        GDALGetProjectionRef(hSrcDS),
                                        hDstDS,
                                        GDALGetProjectionRef(hDstDS),
                                        FALSE, 0.0, 1);
    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

    //cout << GDALGetProjectionRef(hDstDS) << endl << endl;

    // Initialize and execute the warp operation.
    GDALWarpOperation oOperation;
    oOperation.Initialize( psWarpOptions );
    oOperation.ChunkAndWarpImage( 0, 0,
                                GDALGetRasterXSize( hDstDS ),
                                GDALGetRasterYSize( hDstDS ) );
    GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
    GDALDestroyWarpOptions( psWarpOptions );

    GDALClose( hDstDS );
    GDALClose( hSrcDS );
}

void ReprojectionWithCreation(const string& srcName, string dstPath)
{
    GDALDriverH hDriver = nullptr;
    GDALDataType eDT;

    GDALDatasetH hSrcDS = nullptr;
    GDALDatasetH hDstDS = nullptr;

    hSrcDS = GDALOpen(srcName.c_str(), GA_ReadOnly);
    //CPLAssert(hSrcDS != NULL)
    if (hSrcDS == nullptr)
    {
        cout << "ReprojectionWithCreation: hSrcDs is null" << endl;
        return;
    }

    eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1));
    hDriver = GDALGetDriverByName( "GTiff" );

    const char* pszSrcWKT = nullptr;
    char* pszDstWKT = nullptr;

    pszSrcWKT = GDALGetProjectionRef(hSrcDS);
    //Check empty or not

    OGRSpatialReference oSRS;
    oSRS.SetUTM(UTMZone, TRUE);
    oSRS.SetWellKnownGeogCS("WGS84");
    oSRS.exportToWkt(&pszDstWKT);

    void *hTransformArg = nullptr;
    hTransformArg = GDALCreateGenImgProjTransformer(
        hSrcDS, pszSrcWKT, NULL, pszDstWKT, FALSE, 0, 1);

    double adfDstGeoTransform[6];
    
    int nPixels = 0;
    int nLines = 0;

    CPLErr eErr;
    eErr = GDALSuggestedWarpOutput(
        hSrcDS, GDALGenImgProjTransform, hTransformArg,
        adfDstGeoTransform, &nPixels, &nLines);
    GDALDestroyGenImgProjTransformer(hTransformArg);


    // Создание нового tiff
    int pos = srcName.find_last_of("/");
    dstPath += srcName.substr(pos);
    hDstDS = GDALCreate(
        hDriver, dstPath.c_str(), nPixels, nLines, 
        GDALGetRasterCount(hSrcDS), eDT, NULL);

    // hDstDS = GDALCreateCopy(
    //     GDALGetDriverByName("GTiff") , 
    //     dstPath.c_str(), hSrcDS, FALSE, 
    //     NULL, NULL, NULL);

    GDALSetProjection(hDstDS, pszDstWKT);
    GDALSetGeoTransform(hDstDS, adfDstGeoTransform);

    GDALRasterBandH srcRB = GDALGetRasterBand(hSrcDS,1);
    GDALRasterBandH dstRB = GDALGetRasterBand(hDstDS,1);

    GDALColorTableH hCT;
    hCT = GDALGetRasterColorTable(srcRB);
    if( hCT != NULL )
        GDALSetRasterColorTable(dstRB, hCT);


    int srcX = GDALGetRasterBandXSize(srcRB);
    int srcY = GDALGetRasterBandYSize(srcRB);
    GByte buffer[nPixels * nLines];

    GDALRasterIO(srcRB, GF_Read,
        0, 0, srcX, srcY, 
        buffer, nPixels, nLines, 
        eDT, 0, 0);

    GDALRasterIO(dstRB, GF_Write, 
        0, 0, nPixels, nLines, buffer, 
        nPixels, nLines, eDT, 0, 0);

    GDALClose(hSrcDS);
    GDALClose(hDstDS);
}

void ReprojectionWithCreation2(const string& srcName, string dstPath)
{
    GDALDriverH hDriver = nullptr;
    GDALDataType eDT;

    GDALDatasetH hSrcDS = nullptr;
    GDALDatasetH hDstDS = nullptr;

    hSrcDS = GDALOpen(srcName.c_str(), GA_ReadOnly);
    //CPLAssert(hSrcDS != NULL)
    if (hSrcDS == nullptr)
    {
        cout << "ReprojectionWithCreation: hSrcDs is null" << endl;
        return;
    }

    eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1));
    hDriver = GDALGetDriverByName( "GTiff" );

    const char* pszSrcWKT = nullptr;
    char* pszDstWKT = nullptr;

    pszSrcWKT = GDALGetProjectionRef(hSrcDS);
    //Check empty or not

    OGRSpatialReference oSRS;
    oSRS.SetUTM(UTMZone, TRUE);
    oSRS.SetWellKnownGeogCS("WGS84");
    oSRS.exportToWkt(&pszDstWKT);

    void *hTransformArg = nullptr;
    hTransformArg = GDALCreateGenImgProjTransformer(
        hSrcDS, pszSrcWKT, NULL, pszDstWKT, FALSE, 0, 1);

    double adfDstGeoTransform[6];
    
    int nPixels = 0;
    int nLines = 0;

    CPLErr eErr;
    eErr = GDALSuggestedWarpOutput(
        hSrcDS, GDALGenImgProjTransform, hTransformArg,
        adfDstGeoTransform, &nPixels, &nLines);
    GDALDestroyGenImgProjTransformer(hTransformArg);


    // Создание нового tiff
    int pos = srcName.find_last_of("/");
    dstPath += srcName.substr(pos);
    hDstDS = GDALCreateCopy(
        hDriver, dstPath.c_str(), hSrcDS, FALSE, NULL, NULL, NULL);

    GDALSetProjection(hDstDS, pszDstWKT);
    GDALSetGeoTransform(hDstDS, adfDstGeoTransform);

    GDALClose(hSrcDS);
    GDALClose(hDstDS);
}

void ParseSrcFileName(const string& name, double& x, double& y)
{
    const char nums[]
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    int cPos0 = name.find_first_of(nums);
    int cPos1 = name.find_first_not_of(nums);
    string strX = name.substr(cPos0, cPos1);

    int cPos2 = name.find_first_of(nums, cPos1 + 1);
    int cPos3 = name.find_first_not_of(nums, cPos2 + 1);
    string strY = name.substr(cPos2, cPos3);

    x = atof(strX.c_str());
    y = atof(strY.c_str());
}