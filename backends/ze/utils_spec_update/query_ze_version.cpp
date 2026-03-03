#include <iostream>
#include <level_zero/loader/ze_loader.h>
#include <level_zero/ze_api.h>
#include <vector>

int main() {
  // Initialize Level Zero
  zeInit(ZE_INIT_FLAG_GPU_ONLY);

  // Get the driver
  uint32_t driverCount = 1;
  ze_driver_handle_t driver;
  zeDriverGet(&driverCount, &driver);

  // Query driver properties
  ze_driver_properties_t driverProps{.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES};
  zeDriverGetProperties(driver, &driverProps);

  std::cout << "Driver version: " << ZE_MAJOR_VERSION(driverProps.driverVersion) << "."
            << ZE_MINOR_VERSION(driverProps.driverVersion) << std::endl;

  // Compare with header version
  std::cout << "API version: " << ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT) << "."
            << ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT) << std::endl;

  // Query loader component versions
  size_t numElems = 0;
  zelLoaderGetVersions(&numElems, nullptr);
  std::vector<zel_component_version_t> versions(numElems);
  zelLoaderGetVersions(&numElems, versions.data());

  std::cout << "Loader component versions:" << std::endl;
  for (size_t i = 0; i < numElems; ++i) {
    std::cout << "  [" << i << "] Name:        " << versions[i].component_name << std::endl
              << "      Spec:        " << ZE_MAJOR_VERSION(versions[i].spec_version) << "."
              << ZE_MINOR_VERSION(versions[i].spec_version) << std::endl
              << "      Lib version: " << versions[i].component_lib_version.major << "."
              << versions[i].component_lib_version.minor << "."
              << versions[i].component_lib_version.patch << std::endl;
  }

  return 0;
}
