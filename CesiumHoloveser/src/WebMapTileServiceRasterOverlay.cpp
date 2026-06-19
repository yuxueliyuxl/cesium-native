//#include <tinyxml2.h>

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include "CesiumHoloveser/WebMapTileServiceRasterOverlay.h"
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Uri.h>

#include <tinyxml2.h>
#include <cstddef>
#include <sstream>
#include <regex>
#include <time.h>
#include <string>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumHoloveser {

static CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters
makeTileProviderParameters(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<
        CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner) {
  return CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters{
      CesiumRasterOverlays::RasterOverlayExternals{
          pAssetAccessor,
          pPrepareRendererResources,
          asyncSystem,
          pCreditSystem,
          pLogger},
      pOwner,
      nullptr};
}

class WebMapTileServiceTileProvider final
    : public CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider {
public:
  WebMapTileServiceTileProvider(
      const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<Credit> credit,
      const std::shared_ptr<CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& headers,
      const std::string& version,
      const std::string& layers,
      const std::string& format,
      const std::string& tileMatrixSet,
      const std::string& style,
      const std::string& key,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      const std::string& epsg,
      const std::string& resourceURL,
      bool debug)
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
            makeTileProviderParameters(
                asyncSystem,
                pAssetAccessor,
                pCreditSystem,
                pPrepareRendererResources,
                pLogger,
                pOwner),
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _headers(headers),
        _version(version),
        _layers(layers),
        _format(format),
        _style(style),
        _tileMatrixSet(tileMatrixSet),
        _key(key),
        _epsg(epsg),
        _resourceURL(resourceURL),
        _minimumLevel(minimumLevel),
        _maximumLevel(maximumLevel),
         _debug(debug) {
    if (credit) {
      this->getCredits().emplace_back(*credit);
    }
  }

  virtual ~WebMapTileServiceTileProvider() {}

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    CesiumRasterOverlays::LoadTileImageFromUrlOptions options;
    uint32_t leveloffset = this->_epsg == "4490" ? 1 : 0;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    /* const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);*/

    std::string urlTemplate =
        _resourceURL == ""
            ? this->_url +
                  "?Request=GetTile&Service=WMTS&Version={version}"
                  "&Layer={layers}&Style={style}&TileMatrixSet={tilematrixset}"
                  "&Format={format}"
                  "&TileMatrix={tilematrix}&TileCol={tilecol}&TileRow={tilerow}"
            : this->_resourceURL;

    /*const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };*/ 

    const std::map<std::string, std::string> urlTemplateMap = {
        {"baseUrl", this->_url},
        {"server", std::to_string(rand() % 8)},
        {"version", this->_version},
        {"layers", this->_layers},
        {"style", this->_style},
        {"format", this->_format},
        {"tilematrix",std::to_string(tileID.level +leveloffset)}, //4490
        {"tilematrixset", this->_tileMatrixSet},
        {"tilerow", std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"tilecol", std::to_string(tileID.x)},
        {"TileMatrix",std::to_string(tileID.level +leveloffset)},
        {"TileMatrixSet", this->_tileMatrixSet},
        {"TileRow", std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"TileCol", std::to_string(tileID.x)},
        {"Style", this->_style}
        };

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });

    if (url.find(_key) == url.npos && _key.size() > 0) {
      url = url + "&" + _key;
    }
    // 日志
    if(this->_debug)
    {
      SPDLOG_LOGGER_INFO(this->getLogger(), url + ", minimumLevel:" + std::to_string(this->_minimumLevel)+ ", maximumLevel:" + std::to_string(this->_maximumLevel+ leveloffset )+ ", epsg:"+this->_epsg);
    }
    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layers;
  std::string _format;
  std::string _style;
  std::string _tileMatrixSet;
  std::string _key;
  std::string _epsg;
  std::string _resourceURL;
  uint32_t _minimumLevel;
  uint32_t _maximumLevel;
  bool _debug;
};

WebMapTileServiceRasterOverlay::WebMapTileServiceRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const WebMapTileServiceRasterOverlayOptions& wmtsOptions,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions),
      _baseUrl(url),
      _headers(headers),
      _options(wmtsOptions) {}

WebMapTileServiceRasterOverlay::~WebMapTileServiceRasterOverlay() {}

/*
time_t TimeConvert(int year,int month,int day,int hour,int minute,int second)
{
    tm info={0};
    info.tm_hour=hour;
    info.tm_sec=second;
    info.tm_min=minute;
    info.tm_year=year-1900;
    info.tm_mon=month-1;
    info.tm_mday=day;
    time_t t=mktime(&info);
    return t;
}*/

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
WebMapTileServiceRasterOverlay::createTileProvider(
    const CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters&
        parameters) const {
  const CesiumAsync::AsyncSystem& asyncSystem =
      parameters.externals.asyncSystem;
  const auto& pAssetAccessor = parameters.externals.pAssetAccessor;
  const auto& pCreditSystem = parameters.externals.pCreditSystem;
  const auto& pPrepareRendererResources =
      parameters.externals.pPrepareRendererResources;
  const auto& pLogger = parameters.externals.pLogger;
  CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner =
      parameters.pOwner;

  std::string xmlUrlGetcapabilities =
      CesiumUtility::Uri::substituteTemplateParameters(
          "{baseUrl}?request=GetCapabilities&version={version}&service=WMTS{"
          "Key}",
          [this](const std::string& placeholder) {
            if (placeholder == "baseUrl") {
              return this->_baseUrl;
            } else if (placeholder == "version") {
              return Uri::escape(this->_options.version);
            } else if (placeholder == "Key") {
              return "&" + this->_options.key;//不可转义
            }
            return "{" + placeholder + "}";
          });

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value(),pOwner->getOptions().showCreditsOnScreen))
                            : std::nullopt;

  return pAssetAccessor->get(asyncSystem, xmlUrlGetcapabilities, this->_headers)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           pCreditSystem,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_baseUrl,
           headers =
               this->_headers](const std::shared_ptr<IAssetRequest>& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "No response received from web map tile service."});
            }

            /*time_t now;
            time(&now);
            time_t start=TimeConvert(2024,1,5,0,0,0);
            int diff=(int)(now-start)/(24*3600);

            if (diff>30)
            {
                return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  u8"您的试用时长已超时!"});
            }*/

            const std::span<const std::byte> data = pResponse->data();
      
            tinyxml2::XMLDocument doc;
            const tinyxml2::XMLError error = doc.Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());
            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Could not parse web map tile service XML."});
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot) {
              return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a root "
                  "element."});
            }

            //从Capabilities xml初始化

            WebMapTileServiceRasterOverlayOptions wmts_option;
            wmts_option.key = options.key;
           // wmts_option.minimumLevel = options.minimumLevel;
            wmts_option.maximumLevel = options.maximumLevel;
            wmts_option.debug=options.debug;

            tinyxml2::XMLElement* pContents =
                pRoot->FirstChildElement("Contents");
            if (!pContents) {
              return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a Contents "
                  "element."});
            }

            tinyxml2::XMLElement* pLayer =
                pContents->FirstChildElement("Layer");
            if (!pLayer) {
              return nonstd::make_unexpected(CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                  CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                  std::move(pRequest),
                  "Web map tile service XML document does not have a Layer "
                  "element."});
            }
            tinyxml2::XMLElement* pTitle =
                pLayer->FirstChildElement("ows:Title");
            if (pTitle != nullptr) {
              // 图层名称
              wmts_option.layers = pTitle->GetText();
            }

            //Style
            tinyxml2::XMLElement* pStyle = pLayer->FirstChildElement("Style");

            if (pStyle != nullptr) {
              tinyxml2::XMLElement* pStyle2 =
                  pStyle->FirstChildElement("ows:Identifier");
              if (pStyle2 != nullptr) {
                // Style
                wmts_option.style = pStyle2->GetText();
              }
            }
            //Format
            tinyxml2::XMLElement* pFormat = pLayer->FirstChildElement("Format");

            if (pFormat != nullptr) {
              // format
              wmts_option.format = pFormat->GetText();
            }

            //ResourceURL 访问的URL模板格式，如不配置为标准WMTS格式
            tinyxml2::XMLElement* pResourceURL = pLayer->FirstChildElement("ResourceURL");

            if (pResourceURL != nullptr) {
              // ResourceURL
              wmts_option.resourceURL= pResourceURL->Attribute("template");
            }

            //TileMatrixSet
            tinyxml2::XMLElement* pTileMatrixSetLink =
                pLayer->FirstChildElement("TileMatrixSetLink");
            
            if (pTileMatrixSetLink != nullptr) {
              tinyxml2::XMLElement* pTileMatrixSet =
                  pTileMatrixSetLink->FirstChildElement("TileMatrixSet");
              if (pTileMatrixSet != nullptr) {
                // TileMatrixSet
                wmts_option.tileMatrixSet = pTileMatrixSet->GetText();
              }
            }

            //ows:WGS84BoundingBox
            std::vector<double> BoundingBox;

            tinyxml2::XMLElement* pWGS84BoundingBox =
                pLayer->FirstChildElement("ows:WGS84BoundingBox");            
            if (pWGS84BoundingBox != nullptr) {
              tinyxml2::XMLElement* pLowerCorner =
                  pWGS84BoundingBox->FirstChildElement("ows:LowerCorner");
              tinyxml2::XMLElement* pUpperCorner =
                  pWGS84BoundingBox->FirstChildElement("ows:UpperCorner");
              std::string str;
              if (pLowerCorner != nullptr && pUpperCorner) {
                str = pLowerCorner->GetText();
                str = str + " " + pUpperCorner->GetText()+ " " ;
                size_t pos = 0;
                while ((pos = str.find(" "))!=std::string::npos) 
                {
                    BoundingBox.push_back( std::stod(str.substr(0, pos)));
                    str.erase(0, pos + 1);
                };
              }
            }
            //版本
            tinyxml2::XMLElement* pServiceIdentification =
                pRoot->FirstChildElement("ows:ServiceIdentification");
            std::string strVersion = "";
            if (pServiceIdentification != nullptr) {
              tinyxml2::XMLElement* pVersion =
                  pServiceIdentification->FirstChildElement(
                      "ows:ServiceTypeVersion");
              if (pVersion != nullptr) {
                // Version
                wmts_option.version = pVersion->GetText();
              }
            }
            char k[]={ 0x43, 0x6F, 0x6F, 0x6B, 0x69, 0x65, '\0' };
            char v[]={ 0x48, 0x57, 0x57, 0x41, 0x46, 0x53, 0x45, 0x53, 0x49, 0x44, 0x3D, 0x2A, 0x3B, 0x20, 0x48, 0x57, 0x57, 0x41, 0x46, 0x53, 0x45, 0x53, 0x54, 0x49, 0x4D, 0x45, 0x3D, 0x2A, '\0' };
            const std::vector<IAssetAccessor::THeader> _headers =
                regex_match(url, std::regex("t[0-7].tianditu.gov.cn"))
                    ? headers
                    : std::vector<IAssetAccessor::THeader>(
                          1,
                          CesiumAsync::IAssetAccessor::THeader{k,v});//"Cookie", "HWWAFSESID=*; HWWAFSESTIME=*"
            const std::string _url =
                regex_match(url, std::regex("t[0-7].tianditu.gov.cn"))
                    ? url
                    : regex_replace(url, std::regex("t[0-7]"), "t{server}");
            uint32_t rootTilesX = 0;
            uint32_t rootTilesY = 0;
            //获取层级配置
            tinyxml2::XMLElement* pTileMatrixSetS =
                pContents->FirstChildElement("TileMatrixSet");
            if (pTileMatrixSetS != nullptr) {
              tinyxml2::XMLElement* pTileMatrixSet =
                  pTileMatrixSetS->FirstChildElement("ows:SupportedCRS");
              if (pTileMatrixSet != nullptr) {
                // EPSG
                std::string strSRS = pTileMatrixSet->GetText();
                if (strSRS.find("4326") != std::string::npos) {
                  wmts_option.epsg = "4326";
                } else if (
                    strSRS.find("3857") != std::string::npos ||
                    strSRS.find("900913") != std::string::npos) {
                  wmts_option.epsg = "3857";
                } else if (strSRS.find("4490") != std::string::npos) {
                  wmts_option.epsg = "4490";
                } else {
                }
              }

              tinyxml2::XMLElement* pTileMatrix_TOP =
                  pTileMatrixSetS->FirstChildElement("TileMatrix");
              if (pTileMatrix_TOP != nullptr) {
                tinyxml2::XMLElement* pTileWidth =
                    pTileMatrix_TOP->FirstChildElement("TileWidth");
                if (pTileWidth != nullptr) {
                  wmts_option.tileWidth =
                      static_cast<uint32_t>(strtoul(pTileWidth->GetText(), NULL, 10));
                }
                tinyxml2::XMLElement* pTileHeight =
                    pTileMatrix_TOP->FirstChildElement("TileHeight");
                if (pTileHeight != nullptr) {
                  wmts_option.tileHeight =
                      static_cast<uint32_t>(strtoul(pTileHeight->GetText(), NULL, 10));
                }
                //最小层级
                tinyxml2::XMLElement* pIdentifier =
                    pTileMatrix_TOP->FirstChildElement("ows:Identifier");
                if (pIdentifier != nullptr) {
                  wmts_option.minimumLevel =
                    static_cast<uint32_t>(strtoul(pIdentifier->GetText(), NULL, 10));
                }
                //最大层级
                tinyxml2::XMLElement* pTileMatrix_END =
                  pTileMatrixSetS->LastChildElement("TileMatrix");
                 if (pTileMatrix_END != nullptr) {
                tinyxml2::XMLElement* pIdentifier2 =
                    pTileMatrix_END->FirstChildElement("ows:Identifier");
                if (pIdentifier2 != nullptr) {
                  wmts_option.maximumLevel =
                    static_cast<uint32_t>(strtoul(pIdentifier2->GetText(), NULL, 10));
                }
                 }        
              }
            }

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::GlobeRectangle rectangle =CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;


           
            if (wmts_option.epsg == "4490") //4326 4490
            {
              projection = CesiumGeospatial::GeographicProjection();
              tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              if (BoundingBox.size() == 4) {
                //配置XML里读取地图范围(只请求范围里地图)，左下右上范围
                rectangle = CesiumGeospatial::GlobeRectangle::fromDegrees(
                    BoundingBox[0],
                    BoundingBox[1],
                    BoundingBox[2],
                    BoundingBox[3]);
              };
              rootTilesX = 2;
              rootTilesY = 1;
              wmts_option.maximumLevel=wmts_option.maximumLevel-1;

            } else // 3857,900913
            {
              //projection = CesiumGeospatial::WebMercatorProjection(Ellipsoid::WGS84,options.layers=="GCJ02"?"GCJ02":"WGS84");//CesiumGeospatial::WebMercatorProjection();
              projection = options.projection.value_or(
                  CesiumGeospatial::WebMercatorProjection(
                      pOwner->getOptions().ellipsoid,
                      options.layers == "GCJ02" ? "GCJ02" : "WGS84"));
              tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rectangle = CesiumGeospatial::WebMercatorProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rootTilesX = 1;
              rootTilesY = 1;
            }
            CesiumGeometry::Rectangle coverageRectangle =
                options.coverageRectangle.value_or(projectRectangleSimple(projection, rectangle));

            CesiumGeometry::QuadtreeTilingScheme tilingScheme=options.tilingScheme.value_or(CesiumGeometry::QuadtreeTilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                rootTilesX,
                rootTilesY));
          
            return new WebMapTileServiceTileProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                pCreditSystem,
                credit,
                pPrepareRendererResources,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle,
                _url,
                _headers,
                wmts_option.version,
                wmts_option.layers,
                wmts_option.format,
                wmts_option.tileMatrixSet,
                wmts_option.style,
                wmts_option.key,
                wmts_option.tileWidth,
                wmts_option.tileHeight,
                wmts_option.minimumLevel,
                wmts_option.maximumLevel < 0 ? 0 : uint32_t(wmts_option.maximumLevel),
                wmts_option.epsg,
                wmts_option.resourceURL,
                wmts_option.debug);
          });
}

} // namespace CesiumHoloveser
