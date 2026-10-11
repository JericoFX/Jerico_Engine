cmake_minimum_required(VERSION 3.24)
project(JEVvlWrongVersion LANGUAGES NONE)
list(PREPEND CMAKE_MODULE_PATH "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package")
find_package(vulkan-validationlayers 1.3.262 EXACT REQUIRED MODULE)
