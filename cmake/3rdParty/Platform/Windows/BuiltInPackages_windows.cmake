#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Windows.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                    TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                      TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                            TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.3.5-je-rev1-windows                            TARGETS zstd                        PACKAGE_HASH 672edc4a41e31c6f45675b84a314cf1a31caa75428c543ea67a27253d9588793)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-je-rev1-windows                           TARGETS expat                       PACKAGE_HASH c102370257ed28d9f9ea5e61ca10432edc14e8389aa781380beaa9b2a8ae6925)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-je-rev1-windows                          TARGETS OpenEXR Imath               PACKAGE_HASH cd06701c7710fcaf68b29d99a1a4e110ea4c274b0656e1cc2182a1a7003f7fa0)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 62fc6978933215440718d8d810ab5ecf20a36368a8592d12822f902ed0654782)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-windows                     TARGETS SPIRVCross                  PACKAGE_HASH 4c18c95834ebff103ff7f39fc814d65276668d2405d91376290f69723f2ab0f7)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-je-rev1-windows                         TARGETS TIFF                        PACKAGE_HASH d54ecd9a2463472e0df48e9baff07744ca1512fe761bc142ade353b347bf1dd2)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-je-rev1-windows                       TARGETS Freetype                    PACKAGE_HASH a6999aa779cd5e37e415eb784188f73763d396eaaca58f5b9485e2a5568b9802)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev1-windows                    TARGETS AWSNativeSDK                PACKAGE_HASH 5112bfc8982f240757f22f32f5f761d7d7020d1c77b029ae480c082fa3ae19d4)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-je-rev1-windows                             TARGETS Lua                         PACKAGE_HASH d3962b7b84f7fb484f02f639f631db5613f30a6922953ce41ce8344e4b57cbeb)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-je-preopen-pathprobe-r1-windows                 TARGETS mcpp                        PACKAGE_HASH 2deaba55570d34409f7d3c9c3394a40997ea2438a602499a5a47fca3654e4503)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-windows                             TARGETS mikkelsen                   PACKAGE_HASH 872c4d245a1c86139aa929f2b465b63ea4ea55b04ced50309135dd4597457a4e)
ly_associate_package(PACKAGE_NAME d3dx12-headers-je-rev1-windows                        TARGETS d3dx12                      PACKAGE_HASH 5ba66c64483f19e72beea0d1ccc94ad3e0175e88ec256668fc5b0725727ffb73)
ly_associate_package(PACKAGE_NAME pyside6-6.11.2-py3.10-rev1-windows                    TARGETS pyside6                     PACKAGE_HASH cd0272eb5ce46a3437132344a4a6b7022e3aee152da888963cdd9d025cbdf506)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-je-rev1-windows        TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenColorIO::Tools::Binaries PACKAGE_HASH 268ecbe940f16f729248679b586542e0154195eb66ae7ac29bccc409e5cf6611)
ly_associate_package(PACKAGE_NAME qt-6.11.2-rev1-windows                                TARGETS Qt                          PACKAGE_HASH f09edc180df0ded92827279c475167be13cbb517de182a5b10ec1337e68dd834)
ly_associate_package(PACKAGE_NAME png-1.6.37-je-rev1-windows                            TARGETS PNG                         PACKAGE_HASH 7d13e9a70172f713f66daf870a7359191096308ebe501c17121bfb24dbefe9b4)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-je-rev2-windows                      TARGETS libsamplerate               PACKAGE_HASH 01086cd19a1d241b4b0504fe16142c80bf29af329543e67ff773301edaa95122)
ly_associate_package(PACKAGE_NAME civetweb-1.8-rev1-windows                             TARGETS civetweb                    PACKAGE_HASH 36d0e58a59bcdb4dd70493fb1b177aa0354c945b06c30416348fd326cf323dd4)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-je-rev1-windows                        TARGETS OpenSSL                     PACKAGE_HASH 122f6d3a20fd4e8bdc158bccae6014f129be3fac7f16704ef01e75b8a2491888)
ly_associate_package(PACKAGE_NAME Crashpad-0.8.0-rev1-windows                           TARGETS Crashpad                    PACKAGE_HASH d162aa3070147bc0130a44caab02c5fe58606910252caf7f90472bd48d4e31e2)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-je-rev1-windows                           TARGETS ZLIB                        PACKAGE_HASH 4ff69c9c7f99c917afce6d04cdf3fc6f2a65c0038abf2c677ef418182851bef7)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-windows                       TARGETS squish-ccr                  PACKAGE_HASH 5c3d9fa491e488ccaf802304ad23b932268a2b2846e383f088779962af2bfa84)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-je-rev1-windows                         TARGETS astc-encoder                PACKAGE_HASH 4eed3f722e8f4cca60bf14684f22fa351d0d59960ed518be04cce30c707949ca)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-je-rev1-windows                      TARGETS ISPCTexComp                 PACKAGE_HASH 8308677642330694ab1c63c74a7aaabf4a0a9633234f998b12e838cda64008df)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-je-rev1-windows                             TARGETS lz4                         PACKAGE_HASH 9e7e44c4e9d70307e22da51ecbfb71934770ca0ec7c4077d5ee739c2a21c2eac)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-windows        	                TARGETS SQLite                      PACKAGE_HASH c1658c8ed5cf0e45d4a5da940c6a6d770b76e0f4f57313b70d0fd306885f015e)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-windows                TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH b03475a9f0f7a7e7c90619fba35f1a74fb2b8f4cd33fa07af99f2ae9e0c079dd)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.4.350-je-sdk-rev1-windows   TARGETS vulkan-validationlayers     PACKAGE_HASH 986f8ec6343b28dd6ad34d2a5aaa456890a8d42f96b8b4903d532cb5d499255e)
