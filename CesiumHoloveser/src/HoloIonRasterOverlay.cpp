#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumHoloveser/HoloIonRasterOverlay.h>
#include <CesiumHoloveser/UrlTemplateRasterOverlay.h>
#include <CesiumHoloveser/WebMapTileServiceRasterOverlay.h>
#include <CesiumRasterOverlays/BingMapsRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <HoloCreditSystem.h>
#include <rapidjson/document.h>
#include <spdlog/fwd.h>
// #include <openssl/hmac.h>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumHoloveser {

static CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters
makeTileProviderParameters(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<
        CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumUtility::IntrusivePointer<
        const CesiumRasterOverlays::RasterOverlay>& pOwner) {
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

HoloIonRasterOverlay::HoloIonRasterOverlay(
    const std::string& name,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    bool debug,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions,
    const std::string& ionAssetEndpointUrl)
    : RasterOverlay(name, overlayOptions),
      _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _ionAssetEndpointUrl(ionAssetEndpointUrl),
      _isdebug(debug) {}

HoloIonRasterOverlay::~HoloIonRasterOverlay() {}

std::unordered_map<std::string, HoloIonRasterOverlay::ExternalAssetEndpoint>
    HoloIonRasterOverlay::endpointCache;

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
HoloIonRasterOverlay::createTileProvider(
    const ExternalAssetEndpoint& endpoint,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<
        CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>
        pOwner) const {
  IntrusivePointer<CesiumRasterOverlays::RasterOverlay> pOverlay = nullptr;
  /* if (endpoint.externalType == "BING") {
    pOverlay = new CesiumRasterOverlays::BingMapsRasterOverlay(
        this->getName(),
        endpoint.url,
        endpoint.key,
        endpoint.mapStyle,
        endpoint.culture); */

  CesiumRasterOverlays::RasterOverlayOptions options;
  // std::string creditString;
  // this->_ionAssetID
  switch (this->_ionAssetID) {
    // 高德影像
    /*http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=1&style=7 为矢量图（含路网、含注记）
    http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=2&style=7 为矢量图（含路网，不含注记）
    http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=1&style=6 为影像底图（不含路网，不含注记）
    http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=2&style=6 为影像底图（不含路网、不含注记）
    http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=1&style=8 为影像路图（含路网，含注记）
    http://wprd0{1-4}.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scl=2&style=8 为影像路网（含路网，不含注记）*/
  case 1: {
    CesiumHoloveser::UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.subdomains.push_back("1");
    urlOptions.subdomains.push_back("2");
    urlOptions.subdomains.push_back("3");
    urlOptions.subdomains.push_back("4");
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.credit.emplace(amap_Credit);
    urlOptions.debug = _isdebug;

    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://wprd0{s}.is.autonavi.com/"
        "appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=1&style=6",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
    // 高德矢量
  case 11: {
    CesiumHoloveser::UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.subdomains.push_back("1");
    urlOptions.subdomains.push_back("2");
    urlOptions.subdomains.push_back("3");
    urlOptions.subdomains.push_back("4");
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.credit.emplace(amap_Credit);
    urlOptions.debug = _isdebug;

    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://wprd0{s}.is.autonavi.com/"
        "appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=1&style=7",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  /* google 中国
  hl=zh-CN（中文）, hl=nl （中英双语）
  lyrs=y:影像标注
  lyrs=s:影像无标注
  lyrs=r:矢量带标注（偏灰）
  lyrs=m:矢量带标注（偏黄）
  lyrs=p:地形融合
  lyrs=t:地形渲晕
  lyrs=h:  路网加注记/交通图（可以和其他图叠加）
  gl=zh-CH:谷歌的TMS在针对中国有两套，一套做了偏移，一套没有。经测试在上面的TMS
  的url加会让切片地图有稍微的偏移，如果谷歌TMS和你的地图有稍微偏差的话，可以尝试加入这个参数对齐地图，切换偏移或无偏移的地图
  注：lyrs=y时,参数中加hl=zh-CN不加gl=zh-CH，会乱：影像为偏移，标注为不偏移
  */
  case 2: {
    // 中英文标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/"
        "vt?lyrs=s,m&gl=CN&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  case 21: {
    //"https://0pn.cn/maps/vt?lyrs=s&gl=CN&x={x}&y={y}&z={z}", 2024.4不能访问服务
    // 影像带标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/vt?lyrs=y&gl=CN&hl=zh-CN&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  case 22: {
    //https://0pn.cn/maps/vt?lyrs=m&x={x}&y={y}&z={z} 2024.4不能访问服务
    // 影像不带标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "WGS84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/vt?lyrs=s&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  case 23: {
    // 矢量带标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/vt?lyrs=m&gl=CN&hl=zh-CN&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  case 24: {
    // 地形融合带标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/vt?lyrs=p&gl=CN&hl=zh-CN&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  case 25: {
    // 中文路网标注
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 22;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(google_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://0414.gggis.com/maps/vt?lyrs=h&gl=CN&hl=zh-CN&x={x}&y={y}&z={z}",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }

    // 天地图影像 经纬度投影
  case 3: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/img_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
    // 天地图影像标注 经纬度投影
  case 31: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/cia_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图矢量底图 经纬度投影
  case 32: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/vec_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图矢量注记 经纬度投影
  case 33: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/cva_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图地形晕渲 经纬度投影
  case 34: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/ter_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图全球境界 经纬度投影
  case 35: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/ibo_c/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
    // 天地图影像 墨卡托投影
  case 7: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/img_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
    // 天地图影像标注 墨卡托投影
  case 71: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/cia_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图矢量底图 墨卡托投影
  case 72: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/vec_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图矢量注记 墨卡托投影
  case 73: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/cva_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图地形晕渲 墨卡托投影
  case 74: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/ter_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图全球境界 墨卡托投影
  case 75: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "tk=2a09154a042849f0b1faa8c5da06aa98";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "http://t0.tianditu.gov.cn/ibo_w/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // 天地图地表覆盖 墨卡托投影
  case 76: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 12;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "wgs84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(tianditu_tcland);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://lcdata.tianditu.gov.cn/glc2020_w/"
        "wmts?SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&LAYER=glc2020&STYLE="
        "default&TILEMATRIXSET=w&FORMAT=tiles&TILEMATRIX={z}&TILEROW={y}&"
        "TILECOL={x}&tk=2a09154a042849f0b1faa8c5da06aa98",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  // 天地图山影 墨卡托投影
  case 77: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 12;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "wgs84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://lcdata.tianditu.gov.cn/terrain-rgb_w/"
        "wmts?SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&LAYER=terrain-rgb&"
        "STYLE=default&TILEMATRIXSET=w&FORMAT=tiles&TILEMATRIX={z}&TILEROW={y}&"
        "TILECOL={x}&tk=2a09154a042849f0b1faa8c5da06aa98",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
    // 星图影像
  case 4: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "WGS84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(geovisearth_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://tiles.geovisearth.com/base/v1/img/{z}/{x}/"
        "{y}?format=webp&token="
        "d8cdb82643a515ac484729f6c36633046e9592c1dc19ee060d23e9f73ee291b8",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
    // 星图矢量
  case 41: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "WGS84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(geovisearth_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://tiles.geovisearth.com/base/v1/vec/{z}/{x}/"
        "{y}?format=png&tmsIds=w&token="
        "d8cdb82643a515ac484729f6c36633046e9592c1dc19ee060d23e9f73ee291b8",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
    // 星图地形
  case 42: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 12;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "WGS84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(geovisearth_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://tiles1.geovisearth.com/base/v1/ter/{z}/{x}/"
        "{y}?format=png&tmsIds=w&token="
        "d8cdb82643a515ac484729f6c36633046e9592c1dc19ee060d23e9f73ee291b8",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
    // 星图影像标注
  case 43: {
    UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "WGS84";
    urlOptions.debug = _isdebug;
    urlOptions.credit.emplace(geovisearth_Credit);
    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "https://tiles.geovisearth.com/base/v1/cia/{z}/{x}/"
        "{y}?format=png&tmsIds=w&token="
        "d8cdb82643a515ac484729f6c36633046e9592c1dc19ee060d23e9f73ee291b8",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  // MAPBOX影像
  case 5: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "access_token=pk."
                      "eyJ1IjoieXV4dWVsaSIsImEiOiJjbGJreDlmamIwMjJ5M29vOXNxbGZ2"
                      "cGk0In0.e32KNK1EeQmgz1-BaMOfQg";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(holoveser_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://api.mapbox.com/styles/v1/yuxueli/clipl9x28009t01r82x4i2lr0/"
        "wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // MAPBOX 矢量
  case 51: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "access_token=pk."
                      "eyJ1IjoieXV4dWVsaSIsImEiOiJjbGJreDlmamIwMjJ5M29vOXNxbGZ2"
                      "cGk0In0.e32KNK1EeQmgz1-BaMOfQg";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(holoveser_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://api.mapbox.com/styles/v1/yuxueli/cln5kfw5y006n01q53niq9hym/"
        "wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // MAPBOX用地分布
  case 52: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "access_token=pk."
                      "eyJ1IjoieXV4dWVsaSIsImEiOiJjbGJreDlmamIwMjJ5M29vOXNxbGZ2"
                      "cGk0In0.e32KNK1EeQmgz1-BaMOfQg";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(holoveser_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://api.mapbox.com/styles/v1/yuxueli/clnif7tx7000o01oi30897irh/"
        "wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  // MAPBOX彩色
  case 53: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.key = "access_token=pk."
                      "eyJ1IjoieXV4dWVsaSIsImEiOiJjbGJreDlmamIwMjJ5M29vOXNxbGZ2"
                      "cGk0In0.e32KNK1EeQmgz1-BaMOfQg";
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(holoveser_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://api.mapbox.com/styles/v1/yuxueli/clo6wbxle009w01rf9vb3ge8l/"
        "wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
    // 江苏天地图
  case 6: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://jiangsu.tianditu.gov.cn/historyraster/rest/services/"
        "historyVector/js_sldt_blue/MapServer/WMTS",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  case 61: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://jiangsu.tianditu.gov.cn/historyraster/rest/services/"
        "historyVector/js_sldt_black/MapServer/WMTS",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  case 62: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://jiangsu.tianditu.gov.cn/historyraster/rest/services/"
        "historyVector/js_sldt_grey/MapServer/WMTS",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
  case 63: {
    WebMapTileServiceRasterOverlayOptions wmtsOptions;
    wmtsOptions.debug = _isdebug;
    wmtsOptions.credit.emplace(tianditu_Credit);
    pOverlay = new CesiumHoloveser::WebMapTileServiceRasterOverlay(
        this->getName(),
        "https://jiangsu.tianditu.gov.cn/mapjs2/rest/services/MapJS/"
        "js_yxdt_latest/MapServer/wmts",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        wmtsOptions,
        this->getOptions());
    break;
  }
    // 自定义URL
  case 99: {
    CesiumHoloveser::UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = _isdebug ? "GCJ02" : "WGS84";

    urlOptions.debug = true;

    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        _ionAccessToken,
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  default: {
    CesiumHoloveser::UrlTemplateRasterOverlayOptions urlOptions;
    urlOptions.subdomains.push_back("1");
    urlOptions.subdomains.push_back("2");
    urlOptions.subdomains.push_back("3");
    urlOptions.subdomains.push_back("4");
    urlOptions.maximumLevel = 18;
    urlOptions.minimumLevel = 0;
    urlOptions.layers = "GCJ02";
    urlOptions.credit.emplace(amap_Credit);
    urlOptions.debug = _isdebug;

    pOverlay = new CesiumHoloveser::UrlTemplateRasterOverlay(
        this->getName(),
        "http://wprd0{s}.is.autonavi.com/"
        "appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=1&style=6",
        std::vector<CesiumAsync::IAssetAccessor::THeader>(),
        urlOptions,
        this->getOptions());
    break;
  }
  }
  (void)endpoint;
  // if (endpoint.url.empty()) {
  //}

  // if (pCreditSystem) {
  //  std::vector<Credit>& credits = pOverlay->getCredits();
  /*for (const auto& attribution : endpoint.attributions) {
   credits.emplace_back(pCreditSystem->createCredit(
       attribution.html,
       !attribution.collapsible || this->getOptions().showCreditsOnScreen));
 } */

  // credits.emplace_back(pCreditSystem->createCredit(creditString,true));//pOwner->getOptions().showCreditsOnScreen
  //}

  return pOverlay->createTileProvider(makeTileProviderParameters(
      asyncSystem,
      pAssetAccessor,
      pCreditSystem,
      pPrepareRendererResources,
      pLogger,
      pOwner));
}

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
HoloIonRasterOverlay::createTileProvider(
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
  std::string ionUrl = this->_ionAssetEndpointUrl + "v1/assets/" +
                       std::to_string(this->_ionAssetID) + "/endpoint";
  ionUrl = CesiumUtility::Uri::addQuery(
      ionUrl,
      "access_token",
      this->_ionAccessToken);

  pOwner = pOwner ? pOwner : this;

  auto cacheIt = HoloIonRasterOverlay::endpointCache.find(ionUrl);

  // zzt
  if (ionUrl != "1") {
    return createTileProvider(
        cacheIt->second,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        pPrepareRendererResources,
        pLogger,
        pOwner);
  }
  // 缓存中存在
  if (cacheIt != HoloIonRasterOverlay::endpointCache.end()) {
    return createTileProvider(
        cacheIt->second,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        pPrepareRendererResources,
        pLogger,
        pOwner);
  }
  // 缓存中不存在，建立请求
  return pAssetAccessor->get(asyncSystem, ionUrl)
      .thenImmediately(
          [](std::shared_ptr<IAssetRequest>&& pRequest)
              -> nonstd::expected<
                  ExternalAssetEndpoint,
                  CesiumRasterOverlays::RasterOverlayLoadFailureDetails> {
            const IAssetResponse* pResponse = pRequest->response();

            rapidjson::Document response;
            response.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            /* if (response.HasParseError()) {
                return nonstd::make_unexpected(
                    CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                        CesiumRasterOverlays::RasterOverlayLoadType::CesiumIon,
                        std::move(pRequest),
                        fmt::format(
                          "Error while parsing Cesium ion raster overlay
              response, "
                          "error code {} at byte offset {}",
                          response.GetParseError(),
                          response.GetErrorOffset())});
              }*/

            std::string type =
                JsonHelpers::getStringOrDefault(response, "type", "unknown");
            if (type != "IMAGERY") {
              return nonstd::make_unexpected(
                  CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                      CesiumRasterOverlays::RasterOverlayLoadType::CesiumIon,
                      std::move(pRequest),
                      fmt::format(
                          "Assets used with a raster overlay must have type "
                          "'IMAGERY', but instead saw '{}'.",
                          type)});
            }

            ExternalAssetEndpoint endpoint;
            endpoint.externalType = JsonHelpers::getStringOrDefault(
                response,
                "externalType",
                "unknown");
            if (endpoint.externalType == "BING") {
              const auto optionsIt = response.FindMember("options");
              if (optionsIt == response.MemberEnd() ||
                  !optionsIt->value.IsObject()) {
                return nonstd::make_unexpected(
                    CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                        CesiumRasterOverlays::RasterOverlayLoadType::CesiumIon,
                        std::move(pRequest),
                        fmt::format("Cesium ion Bing Maps raster overlay "
                                    "metadata response "
                                    "does not contain 'options' or it is not "
                                    "an object.")});
              }

              const auto attributionsIt = response.FindMember("attributions");
              if (attributionsIt != response.MemberEnd() &&
                  attributionsIt->value.IsArray()) {

                for (const rapidjson::Value& attribution :
                     attributionsIt->value.GetArray()) {
                  AssetEndpointAttribution& endpointAttribution =
                      endpoint.attributions.emplace_back();
                  const auto html = attribution.FindMember("html");
                  if (html != attribution.MemberEnd() &&
                      html->value.IsString()) {
                    endpointAttribution.html = html->value.GetString();
                  }
                  auto collapsible = attribution.FindMember("collapsible");
                  if (collapsible != attribution.MemberEnd() &&
                      collapsible->value.IsBool()) {
                    endpointAttribution.collapsible =
                        collapsible->value.GetBool();
                  }
                }
              }

              const auto& options = optionsIt->value;
              endpoint.url =
                  JsonHelpers::getStringOrDefault(options, "url", "");
              endpoint.key =
                  JsonHelpers::getStringOrDefault(options, "key", "");
              endpoint.mapStyle = JsonHelpers::getStringOrDefault(
                  options,
                  "mapStyle",
                  "AERIAL");
              endpoint.culture =
                  JsonHelpers::getStringOrDefault(options, "culture", "");
            } else {
              endpoint.url =
                  JsonHelpers::getStringOrDefault(response, "url", "");
              endpoint.accessToken =
                  JsonHelpers::getStringOrDefault(response, "accessToken", "");
            }

            return endpoint;
          })
      .thenInMainThread(
          [asyncSystem,
           pOwner,
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           ionUrl,
           this,
           pLogger](nonstd::expected<
                    ExternalAssetEndpoint,
                    CesiumRasterOverlays::RasterOverlayLoadFailureDetails>&&
                        result) -> Future<CreateTileProviderResult> {
            if (result) {
              HoloIonRasterOverlay::endpointCache[ionUrl] = *result;
              return this->createTileProvider(
                  *result,
                  asyncSystem,
                  pAssetAccessor,
                  pCreditSystem,
                  pPrepareRendererResources,
                  pLogger,
                  pOwner);
            } else {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(std::move(result).error()));
            }
          });
}
} // namespace CesiumHoloveser
