#pragma once

/**
 * @brief Classes for raster overlays, which allow draping massive 2D textures
 * over a model.
 */
namespace CesiumHoloveser {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMHOLOVESER_BUILDING
#define CESIUMHOLOVESER_API __declspec(dllexport)
#else
#define CESIUMHOLOVESER_API __declspec(dllimport)
#endif
#else
#define CESIUMHOLOVESER_API
#endif
