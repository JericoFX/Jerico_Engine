/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/View.h>

#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

namespace UnitTest
{
    using namespace AZ;

    class RenderPipelineTests
        : public RPITestFixture
    {

    protected:

        // A test pass which only include view tag and draw list tag
        class TestPass
            : public JE::Render::RPI::ParentPass
        {
        public:

            AZ_RTTI(TestPass, "{2056532E-286F-454F-8659-15A289432A63}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(TestPass, AZ::SystemAllocator);

            TestPass(const JE::Render::RPI::PassDescriptor& descriptor)
                : JE::Render::RPI::ParentPass(descriptor)
            { }

            void Initialize(const Name& drawListTagString, const JE::Render::RPI::PipelineViewTag& viewTag)
            {
                RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
                m_drawListTag = drawListTagRegistry->AcquireTag(drawListTagString);
                m_viewTag = viewTag;
                m_flags.m_hasDrawListTag = true;
                m_flags.m_bindViewSrg = true;
            }

            AZ::RHI::DrawListTag GetDrawListTag() const override
            {
                return m_drawListTag;
            }

            const JE::Render::RPI::PipelineViewTag& GetPipelineViewTag() const override
            {
                return m_viewTag;
            }

            virtual ~TestPass()
            {
                if (m_drawListTag.IsValid())
                {
                    RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
                    drawListTagRegistry->ReleaseTag(m_drawListTag);
                }
            }

        private:
            AZ::RHI::DrawListTag m_drawListTag;
            JE::Render::RPI::PipelineViewTag m_viewTag;
        };
    };

    /**
     * Validate view and view tag related functions
     */
    TEST_F(RenderPipelineTests, ViewFunctionsTest)
    {
        using namespace AZ;

        RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();

        const JE::Render::RPI::PipelineViewTag viewTag1{ "viewTag1" };
        const JE::Render::RPI::PipelineViewTag viewTag2{ "viewTag2" };
        const JE::Render::RPI::PipelineViewTag viewTag3{ "viewTag3" };
        const Name drawListTagString1{ "drawListTag1" };
        const Name drawListTagString2{ "drawListTag2" };
        const Name drawListTagString3{ "drawListTag3" };

        // Create render pipeline
        JE::Render::RPI::RenderPipelineDescriptor desc;
        desc.m_mainViewTagName = viewTag2.GetStringView();
        desc.m_name = "TestPipeline";
        JE::Render::RPI::RenderPipelinePtr pipeline = JE::Render::RPI::RenderPipeline::CreateRenderPipeline(desc);
        
        auto& rootPass = pipeline->GetRootPass();
        JE::Render::RPI::PassDescriptor passDescriptor;

        passDescriptor.m_passName = "TestPassA";
        JE::Render::RPI::Ptr<TestPass> testPassA = aznew TestPass(passDescriptor);
        testPassA->Initialize(drawListTagString1, viewTag1);

        passDescriptor.m_passName = "TestPassB";
        JE::Render::RPI::Ptr<TestPass> testPassB = aznew TestPass(passDescriptor);
        testPassB->Initialize(drawListTagString1, viewTag2);

        passDescriptor.m_passName = "TestPassC";
        JE::Render::RPI::Ptr<TestPass> testPassC = aznew TestPass(passDescriptor);
        testPassC->Initialize(drawListTagString2, viewTag2);

        passDescriptor.m_passName = "TestPassD";
        JE::Render::RPI::Ptr<TestPass> testPassD = aznew TestPass(passDescriptor);
        testPassD->Initialize(drawListTagString3, viewTag3);

        EXPECT_TRUE(testPassA->HasDrawListTag());
        EXPECT_TRUE(testPassB->HasDrawListTag());
        EXPECT_TRUE(testPassC->HasDrawListTag());
        EXPECT_TRUE(testPassD->HasDrawListTag());

        RHI::DrawListTag drawListTag1 = drawListTagRegistry->FindTag(drawListTagString1);
        RHI::DrawListTag drawListTag2 = drawListTagRegistry->FindTag(drawListTagString2);
        RHI::DrawListTag drawListTag3 = drawListTagRegistry->FindTag(drawListTagString3);

        EXPECT_TRUE(testPassA->GetDrawListTag() == drawListTag1);
        EXPECT_TRUE(testPassB->GetDrawListTag() == drawListTag1);
        EXPECT_TRUE(testPassC->GetDrawListTag() == drawListTag2);
        EXPECT_TRUE(testPassD->GetDrawListTag() == drawListTag3);

        bool skipStateCheckWhenRunningTests = true;
        rootPass->AddChild(testPassA, skipStateCheckWhenRunningTests);
        testPassA->AddChild(testPassB, skipStateCheckWhenRunningTests);
        testPassA->AddChild(testPassC, skipStateCheckWhenRunningTests);
        testPassA->AddChild(testPassD, skipStateCheckWhenRunningTests);

        pipeline->UpdatePasses();

        EXPECT_TRUE(pipeline->HasViewTag(viewTag1));

        auto& views = pipeline->GetViews(viewTag2);

        EXPECT_TRUE(views.size() == 0);

        RHI::DrawListMask drawListMask;
        JE::Render::RPI::PassesByDrawList passesByDrawList;
        // viewTag1 only associates with drawListTag1
        rootPass->GetViewDrawListInfo(drawListMask, passesByDrawList, viewTag1);
        EXPECT_TRUE(drawListMask[drawListTag1.GetIndex()] == true);
        EXPECT_TRUE(drawListMask[drawListTag2.GetIndex()] == false);

        rootPass->GetViewDrawListInfo(drawListMask, passesByDrawList, viewTag2);
        // viewTag2 associates with drawlistTag1 and drawListTag2
        EXPECT_TRUE(drawListMask[drawListTag1.GetIndex()] == true);
        EXPECT_TRUE(drawListMask[drawListTag2.GetIndex()] == true);

        // View functions
        JE::Render::RPI::ViewPtr view1 = JE::Render::RPI::View::CreateView(AZ::Name("testViewA"), JE::Render::RPI::View::UsageCamera);
        JE::Render::RPI::ViewPtr view2 = JE::Render::RPI::View::CreateView(AZ::Name("testViewB"), JE::Render::RPI::View::UsageCamera);
        JE::Render::RPI::ViewPtr view3 = JE::Render::RPI::View::CreateView(AZ::Name("testViewC"), JE::Render::RPI::View::UsageCamera);

        // Persistent view
        pipeline->SetPersistentView(viewTag1, view1);
        EXPECT_TRUE(pipeline->GetViews(viewTag1).size() == 1);
        // Replace persistent view
        pipeline->SetPersistentView(viewTag1, view2);
        auto& viewsFromTag1 = pipeline->GetViews(viewTag1);
        EXPECT_TRUE(viewsFromTag1.size() == 1);
        EXPECT_TRUE(view2 == viewsFromTag1[0]);

        // Try to add a transient view to view tag was associated with persistent view. 
        AZ_TEST_START_ASSERTTEST;
        pipeline->AddTransientView(viewTag1, view1);
        AZ_TEST_STOP_ASSERTTEST(1); 
        EXPECT_TRUE(pipeline->GetViews(viewTag1).size() == 1);

        // Try to register a view with multiple viewTags, persistent or transient
        AZ_TEST_START_ASSERTTEST;
        pipeline->SetPersistentView(viewTag3, view2);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_TRUE(pipeline->GetViews(viewTag3).size() == 0);

        AZ_TEST_START_ASSERTTEST;
        pipeline->AddTransientView(viewTag2, view2);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_TRUE(pipeline->GetViews(viewTag2).size() == 0);

        // overwrite persistent view 2 with view 3;
        pipeline->SetPersistentView(viewTag1, view3);
        auto& viewsFromTag1_2 = pipeline->GetViews(viewTag1);
        EXPECT_TRUE(viewsFromTag1_2.size() == 1);
        EXPECT_TRUE(view3 == viewsFromTag1_2[0]);

        // Transient view
        pipeline->AddTransientView(viewTag2, view1);
        pipeline->AddTransientView(viewTag2, view2);
        auto& viewsFromTag2 = pipeline->GetViews(viewTag2);
        EXPECT_TRUE(viewsFromTag2.size() == 2);
    }

}  // namespace UnitTest
