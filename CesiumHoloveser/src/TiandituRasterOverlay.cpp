#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumHoloveser/TiandituRasterOverlay.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Uri.h>

#include <tinyxml2.h>

#include <cstddef>
#include <sstream>

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

class TiandituTileProvider final
    : public CesiumRasterOverlays::QuadtreeRasterOverlayTileProvider {
public:
  TiandituTileProvider(
      const IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<Credit> credit,
      const std::shared_ptr<
          CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
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
        _baseUrl(url),
        _headers(headers),
        _version(version),
        _layers(layers),
        _format(format),
        _style(style),
        _tileMatrixSet(tileMatrixSet),
        _key(key),
        _epsg(epsg),
        _debug(debug) {
    if (credit) {
      this->getCredits().emplace_back(*credit);
    }
  }

  virtual ~TiandituTileProvider() {}

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
  loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    CesiumRasterOverlays::LoadTileImageFromUrlOptions options;
    uint32_t leveloffset = this->_epsg == "4490" ? 1 : 0;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    /* const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);*/

    const std::string urlTemplate =
        this->_baseUrl +
        "?Request=GetTile&Service=WMTS&Version={version}"
        "&Layer={layers}&Style={style}&TileMatrixSet={tilematrixset}"
        "&Format={format}"
        "&TileMatrix={tilematrix}&TileCol={tilecol}&TileRow={tilerow}&tk={key}";
    /* const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };*/

    const std::map<std::string, std::string> urlTemplateMap = {
        {"server", std::to_string(rand() % 8)},
        {"version", this->_version},
        {"layers", this->_layers},
        {"style", this->_style},
        {"format", this->_format},
        {"tilematrix",
         std::to_string(tileID.level + leveloffset)}, // 4490差一级
        {"tilematrixset", this->_tileMatrixSet},
        {"tilerow",
         std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
        {"tilecol", std::to_string(tileID.x)},
        {"key", this->_key}};

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });

    if (_debug) {
      SPDLOG_LOGGER_INFO(this->getLogger(), url);
    }

    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _baseUrl;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _version;
  std::string _layers;
  std::string _format;
  std::string _style;
  std::string _tileMatrixSet;
  std::string _key;
  std::string _epsg;
  bool _debug;
};

TiandituRasterOverlay::TiandituRasterOverlay(
    const std::string& name,
    const std::vector<IAssetAccessor::THeader>& headers,
    const TiandituRasterOverlayOptions& TDTOptions,
    const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions)
    : CesiumRasterOverlays::RasterOverlay(name, overlayOptions),
      _baseUrl(""),
      _headers(headers),
      _options(TDTOptions) {}

TiandituRasterOverlay::~TiandituRasterOverlay() {}

static TiandituRasterOverlayOptions
setupOption(const TiandituRasterOverlayOptions& options) {
  TiandituRasterOverlayOptions TDT_Options;
  TDT_Options.version = "1.0.0";
  TDT_Options.layers = options.mapStyle;
  TDT_Options.format = "tiles";
  TDT_Options.tileMatrixSet = options.epsg == "4490" ? "c" : "w";
  TDT_Options.style = "default";
  TDT_Options.key = options.key;
  TDT_Options.tileWidth = 255;
  TDT_Options.tileHeight = 255;
  TDT_Options.minimumLevel = options.minimumLevel;
  TDT_Options.maximumLevel = options.maximumLevel;
  TDT_Options.epsg = options.epsg;
  TDT_Options.debug = options.debug;
  return TDT_Options;
}

static std::string setupBaseUrl(const TiandituRasterOverlayOptions& options) {
  std::string url = "https://t{server}.tianditu.gov.cn/";
  url += options.layers;
  url += options.epsg == "4490" ? "_c" : "_w";
  url += "/wmts";
  return url;
}

static std::vector<IAssetAccessor::THeader> setupHeaders() {
  const CesiumAsync::IAssetAccessor::THeader cookie{
      "Cookie",
      "HWWAFSESID=*; HWWAFSESTIME=*"};
  std::vector<IAssetAccessor::THeader> headers_cookie(1, cookie);
  return headers_cookie;
}

static std::string
getXmlUrlGetcapabilities(const TiandituRasterOverlayOptions& options) {
  std::string xmlUrlGetcapabilities = "http://t0.tianditu.gov.cn/";
  xmlUrlGetcapabilities += options.mapStyle;
  xmlUrlGetcapabilities += options.epsg == "4490" ? "_c" : "_w";
  xmlUrlGetcapabilities +=
      "/wmts?request=GetCapabilities&version=1.0.0&service=WMTS";
  return xmlUrlGetcapabilities;
}

static std::string getTiantiduCredit() {
  const std::string credit =
      "<a href=\"https://www.tianditu.gov.cn\"><img "
      "src=\"data:image/"
      "png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAaCAYAAADxNd/"
      "XAAAACXBIWXMAAAsTAAALEwEAmpwYAAAQTUlEQVRYhT1WaXSV5bnd7/"
      "B9Z0zOkHMyk+"
      "kkYUjCXMAgCA5h8NoK2lYUrbYV2tpqq17X1dZ7XbWT1iu1o95qi3LtAE5UERRFBhGZJEBIQB"
      "LIQJKT8czDN7zve3+kvc9az1rPj+d51t77z97MpRFojMCWBA1h8DuXMKlRgk"
      "3LGaRUcOvApmU6zg8rZC2Ff5VbA0p9BG6dwOciSOWBtc0Ma5oYjvZKrG1mWFLDEXITLKyiaG"
      "1gofE0clVBiqCXYPNyBstWSOYJgh4Cl07gcUy1AjCzlKIxQHDdTL35d5t8v33pUf/"
      "2q6rYuqPnzeeFDTAKuHWAVxdPAYrGgfWtzq9vXuW564MT+UORUhpcOZvWKwY+MineCn5m/"
      "5prROhsaj9nKlgCIJh6BgCDowKREAcAOB1AwAeYBI7vry3cF/"
      "TKqksTqSqlFHRGEXADpvh/"
      "PSAlIP6pjy0At0Zqv3OT6wdLm1wbCx2EvbQj+"
      "XhNERn5xQbPN554I7vr7BUZ9VKA5w3AtAGqgEMX7EO/"
      "+KbzhXsb3EuNqAWHTgHOAGIvTxjWhQuD4l2vk6DARfD3T22cvCzhcwGxFPCdNucjC2ewzK6D"
      "ud8BwKc9Es22gpRKLajXWocnRIwxpaQg4CCgZAo0JYBSQCIH6BzImlMk+mPKmcnLE24n+"
      "bf2ntzuvWezP3n"
      "7uML6Zc6fPvZlx707jxhPBr0EfDI1dWDaCp+ct7tWPTp63c7Hwh9AMQIpASYBW+"
      "EvR2zjUJeAxzlFOG8BPiegMDUvivDmr23y31lVQO/"
      "Y2ZFqG4qrdEkhMJlSZlef0TGaItbeswplLoXaIoWkQRGNK8RzgG4AtgScABbWMOgaMJIQXT6"
      "ntjFU56rd/Vby4uGLCvVlBC1lcrjCzyprQxQ+LwFPGgq3L9aRMSUOd0t82Gnt23cqe/"
      "D6efqcUwP54+NJdB/"
      "rNF7+"
      "4Iw4CgCGBWgMmFdFMZkGxjIKOlf4uNPc9bUeced1TfpVK2cAlk0hbYWzccCtE28qKwcBoG2O"
      "Y+GGa9wPV05TVd4C438vDFq/"
      "vzKpsPuMQNoENAJ8dbEGj5s7lrY4H5MpqTI5mbwyAXxhDgHXSN/"
      "pflL7YadEsY+CAcCSCMPiOoqKIEUyK3FtC1/"
      "fNZDbsfm3mU3ne81dq+bTqJFH4YURZXAQ1IYIbllI4aAMkTDFaFLihtnamuUtzrZ4QqI6SEa"
      "TKftoqJDWaUzFrp3tuKk6rE1fUk9bblrsuk0RlVk407XKQ0XeMK3tAEVPVKG5ikVKfMTXE5X"
      "xnSeEaL+Y++3SGu2mRXXaageX2Uwek3tOy87FM/SlvVH7cDIHmwFATZCASwW/"
      "C4hlgW+t8r6YyJBYwMNnbLk38P7ca70/jkXtgsOfW+/"
      "6CyhqwwRFHoKBcQUXI3BwYMMy97PVIa0ynydornWs1jS68NarPM+"
      "1NmorCjy6v9BNKobi1v6vPpe8PZWUxro"
      "V+"
      "oaNz8bXbjssEzqjKC4geGFTwZG8kLHDHebxaUEe2Xit5zuN5dpyByf8S6vcX1GG6DnXax7y+"
      "/kXpALvjYpLHADKivktq1v1NfEJ++LX21zrAi6ne9VCx22rlgq8e8Q8KjvVW2+"
      "fNP8RzwANpcANTRQTaUXDftoyv1ZrXbeo8O6qEF80NmFD1wjau82z1UV8UVGBxssC2rVQwMV"
      "hY3LrB/lH5lXyipd/GNj557cTP+0fU32RYoKjPQLLZvDGWQ3O6c/"
      "8I3Oubb777qc3B/4Mv44/vTr2WHQ0ty+a0UKKij2lRRzSwsTVM/"
      "nCgRF7LweAnlExECmjpbyYl3w+JPNLmxTeOZZ6Zc/"
      "J3L9fHLRHoxOARSmujnAQSOzvFEjkwbfc7X629bqCa5EmmOgzoQjANYWuIWNvU6Vzma6R4ou"
      "DRtzlYP6KIhbY+l1/JhjW3Ifac8e/+T/"
      "5H+kMmFdNUeFXmFfLV4ByPH5r4AWdq2B8zMyePpnvun6u88Gntmffe6fd2DW3jsI"
      "wCbI5cU6j/J7OYQXm4AAHBouZ/ZdtB/J/Pdqjhr5yo/POR7fGbth/Xoy9+oB/"
      "d0mpVhtPiO5yL1KnBgTarwCFTirCbrx9psM04klh+pw06NaJSwiGlhp+VfeY/"
      "VFFgM+OZ1XM42DevlHzOAXElUk5et/"
      "vEoudnIiqMMVYCphMK6xb5PzqgohjacDNigrDxP2TvyceeGJ7atP1zeyap/fYb/"
      "XFZCyeUcjbCpdGxOTKZu1WjaqDdEktw42zOfb3KLxxSmHT"
      "Ne4tiFlI5+SYUyMwpTZ7091FT77wveDnc6frm4cTgE4Av4vijWN27Ok3s//"
      "18+3JtvGkHHc5OALlwIsfpf97xY+Tdz60LX5/"
      "w2ytLOBRpLJIX1Ra5Ij4NFb1zg8D5364Xr+10g8UeQlqiylbUKvfTEDRF83n3v8k8WT/"
      "iPmH79/ofiBFyVBlEIxwwKERzCwlmF+FTMAtxXhaNfJkRqGplOH5AyYAuFesdc/a/"
      "lpsx9l+wLSAp3Ymbn91ur4/7NI9D9wWfF"
      "5SaX3Smf+TkBLTwmzV/"
      "asKfklspvkKtQbGFCZGjPSv9uQfAYAXPzB+43eOd9+"
      "50vN8VUivGhozkm43KSxu0CKpo3TNzlPytf+4yYH7btC85cV6A3wcHceNk/c+n/"
      "3P6aUEVzdRx5pVgXuPnzE+1IX43OfmqAsrSNgQStkBrwrwa+bqX2QcBys9iD+"
      "wPvAKpMIzu7I/"
      "GE8Bbl3h9aPmgTs+zhxau7ZwGUYsgMCyARzvlRjNksFInaMFOkFsSIAXEfzqj5kf"
      "jacgK4IE0RjBlnfF7voS8+FnenL9+8+"
      "anW1zWbHLQe3958QVANhzxsLxXit11zK6bdVSsfTUZfPRxnKgfxzY8k5my5fmuJ96cF1g2/"
      "fXGPd894/puz7uEqM+L4XbJT8emiCKR4pp2YyI/tF7P/"
      "NqlfPcTb98ZvTZ4z1qsNAJ2NaUcT2yLbE6ScRDImkfevL1/"
      "P5Yesq9ExnRce+WicjVTXTVihk8tGOHHP7ZW9aLOgOsjIKAgsOlo2NA7fis28JYGpiII+"
      "XVFaQiAICOfglbQWYzqbukmUfPkIWZ5YCTMPSNC+u595IrvrHW/"
      "fj5AVl4z2rPTiLV57al3o/GzemJnDzAjJQ42TUs+ta0ulYfO5R+feMfMvdTANo/"
      "A5pUgJfBig6ZB/1e2ntpRCJuEEipoblIImvIWDwlTiyuIgff/NT47EQ/"
      "4NIB6SiEbQkUaRZunm1jNMcwkQYW1FIUuCmGEgoDkwoBL0HOBForgbYZAtE4MJ4jMC0Kl5TQ"
      "qd03OiFeefK17EupjDg5"
      "s4pfU9/"
      "g+"
      "FYipeSHHcaveVOlwsFOc887O0f32GwqXVLKkQvPA6QFFe0AdzEUF5mY5heAVLCKVoCEpmPMP"
      "gGfbMeSaQJlAcBb4ABgwgzOhqxuA479Hknux8FYADnzEogCdp9RqA3bKPECc6cRdI8AIE4Ma"
      "8U4FhPYly7F+e44eH4M3uJaRJzdaK6wsKRRw3DUan/pdWvDgkbQXWcgD/"
      "YCTPACTMogKiIVGEwofNbPYTtKoPzVAHdBxi4jUb4Wl9CCM2e7cT"
      "kmYYfmggfLENNq0RfXEWKT2H5ax2uZjTCTVyDd5WCh6RADB2BMa8NZ6xokez9BSBfoGiPgVK"
      "LMR2BJoHdUQfJC9Jd8GXvHF2DQrIUxcgG5VBKJhs3ovNALMzaK3glgwKyCq6oWAkxdTrkwkl"
      "LgJ3u9QLgRPx9aB/"
      "S8D+"
      "AoiEyBZEdANBcAAUE0xGQJYv0agDwYUoARB7OyEC4ftrbXAqlhYGEAhHEQKw1iJgFhgck8BM"
      "tAKAXDBgAFRglMMZViGQBbWYCd"
      "BhwMOknB5nlIJsDZJPLpHP7WDgASKI6gh64H4ldA3aOgoXZQ8DSAFCEamwWnIwwrCeoMVjN/"
      "TTNxBQAATBlhh4s1gtsA0wAwN3EG51BXUbMmUgCNAR7FHLqogZ2Hsg0QzVsLQkAgAnAFZ6Xy"
      "HNIGGktodYET1Q6GKo2CmgAgcmBcL9c9vlaqe2ogbUCYANHraVFZBTTPFA4tzRmzmojunk+"
      "h3MrOgkMpaKGGhz1Vi5/ODB95UPim/dXT9lSPMrNG9uTzHug"
      "FxDXrlhEQSkxvsV9JkXBE1pygSupSmjYtapREPzjLNlNMK5t/"
      "2e7evYwVNbbplYsfN8sX+"
      "rS6GwbsoeRDa1vs1I1znT8qdGHyZLd1omNIGoaphNejDWbKb7rdXbtiqxg9e4QVz5kPy3jZH"
      "Dj8Y3flkotGduxuy0y+rIw0tIrFr2jhxg3ITQD+0sF8emgtJQXl4OGW9VOMqVevX/"
      "MA8ZQykRg4JvoPw7XgW29Q5jCknVO0sEonDl+hFpo+0xw4/BtlpoepMz"
      "BT5ePglVcXKCsLrWr57Y6GtY/"
      "b411DekXr06aj0VOT+PO7m5eL+4pCXHp0ZYW8rGr5LIfn223sy+UeG6qsdRPhDs3qP/"
      "IgmMMLT0kBL5lzIyEE9pUj78jUMAACrWrpBtG772/"
      "5s9vmEX91BQ3OuJkS3Ruk3FVi9eztcNZe/z2qeWeKoRMxq+/Ab2ioMaJXL7853//"
      "xE5R78sQVYDwYWQJpQY/c8BAPNtTlOv663rr0PsSVI2UQ+axjWuu3RWZ8v8xO9rjq1"
      "2wWHTseo8n+eFm4sNlJmefzGDOKS/"
      "TyuhIemFOOBTZzhJXSI0qYtmvRfftlon+3cXrr13h45g/ssc4OMdY5gXwcLFi/"
      "gjp8yJ97fYNIXGknhEHGL39CeTByPwHc1tjZPcxfFRbZkX5QZsJIj7vm3HNUipzgxS1fh8vv"
      "UrnJFq107mPKysHu3b8+"
      "d2prgzBTbwKAmujysPAst8iO9psTF76o1bcts7NR4PL2n2dNkjrclfj1ihnGHevnqsovNitf"
      "XSC/7M0D"
      "4sUhLGxw18yvNM5t3yiyIxREVoJSjYdmtCjIAq1mxf0sNKOVF9XfDDMNVr38K96VP82LaHuf"
      "PXziA6a33HW3HO88InPj3YrwSRm79BdCuIMQZvKyuZXpj56osce6XuWh6dNlvLedMkfEuPju"
      "T8zoyb2serkgegFk9BRI4TQv99UuMj5/+2E7Ez2n+etuMbvfe0kYsX1mQRO/"
      "MC7MoRi5cKRbdI6O5WOn+8Tw059W7MiHl81zumlF7uhzDys7f14Lt8wUk92"
      "HqdNfAd2TdDbddqOY7DkrzXSKV3yhVgvU3SGN5Jns+"
      "TdXqvRglrDqlUB2FOAO0GA9lJUPs4LyChE91cDrVzMZu6RUPl6iVy+"
      "foYRVKdPDmhJGmgbrqqm33AlhOlR6xA1CFJHChOZShLAgrFxM6R6bOLwlQiqVy9I4YsYgiOE"
      "HyQkwZpJQaaGbG06ZGIyq7EQawo5RX7WS0pLIjvaC8nFSUJ42Tr/"
      "cy8KzUtRd3Id8PGFNXEhBGICRBKGhmaV6zYqVYI4CIu"
      "1SmZv0saJGjXAXl+"
      "lhQpgzILOjEiATWuUSoqxsTpkZqPyEUNlxQ1o5A1TLw0qb0kwniRBUSVMpZaehwAkhGgglhD"
      "CmmJOD65pgHka4Q9M49VDmFHAUEOIpLSHcSQilwh6/"
      "6FXpIUXcYQbdYzNPsaasrGkNfxal7qIkQJIgRFijHYf/Dzn17xX7HMjNAAAAAElFTkSuQmCC"
      "\" title=\"tiandigu Imagery\"/></a>";

  return credit;
}

Future<CesiumRasterOverlays::RasterOverlay::CreateTileProviderResult>
TiandituRasterOverlay::createTileProvider(
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

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      std::make_optional(pCreditSystem->createCredit(
          getTiantiduCredit(),
          pOwner->getOptions().showCreditsOnScreen));
  /*const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;*/

  return pAssetAccessor
      ->get(
          asyncSystem,
          getXmlUrlGetcapabilities(this->_options),
          this->_headers)
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

            TiandituRasterOverlayOptions TDT_WMTS_Options;
            std::string TDT_Baseurl;

            if (pResponse) {
              TDT_WMTS_Options = setupOption(options);
              TDT_Baseurl = setupBaseUrl(TDT_WMTS_Options);
            } else {
              return nonstd::make_unexpected(
                  CesiumRasterOverlays::RasterOverlayLoadFailureDetails{
                      CesiumRasterOverlays::RasterOverlayLoadType::TileProvider,
                      std::move(pRequest),
                      "No response received from web map Tile service."});
              ;
            }

            // 初始化投影坐标系
            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;
            uint32_t rootTilesX;
            uint32_t rootTilesY;

            if (options.epsg == "4490") // 4490
            {
              projection = CesiumGeospatial::GeographicProjection();
              tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rootTilesX = 2;
              rootTilesY = 1;
              TDT_WMTS_Options.maximumLevel =
                  TDT_WMTS_Options.maximumLevel -
                  1; // 4490需要maximumLevel-1，否则在裁剪边界时会请求maximumLevel+1级
            } else // 3857,900913
            {
              projection = CesiumGeospatial::WebMercatorProjection(
                  pOwner->getOptions().ellipsoid);
              tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::
                  MAXIMUM_GLOBE_RECTANGLE;
              rootTilesX = 1;
              rootTilesY = 1;
            }

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                rootTilesX,
                rootTilesY);
            //

            return new TiandituTileProvider(
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
                TDT_Baseurl,
                setupHeaders(),
                TDT_WMTS_Options.version,
                TDT_WMTS_Options.layers,
                TDT_WMTS_Options.format,
                TDT_WMTS_Options.tileMatrixSet,
                TDT_WMTS_Options.style,
                TDT_WMTS_Options.key,
                TDT_WMTS_Options.tileWidth,
                TDT_WMTS_Options.tileHeight,
                TDT_WMTS_Options.minimumLevel,
                TDT_WMTS_Options.maximumLevel,
                TDT_WMTS_Options.epsg,
                TDT_WMTS_Options.debug);
          });
}

} // namespace CesiumHoloveser
