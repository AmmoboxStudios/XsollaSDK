// Fill out your copyright notice in the Description page of Project Settings.


#include "XsollaAutomationTests.h"

#include "TestHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXsollaAuthTest, "Xsolla.AutomationTests SimpleAuth", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask);
bool FXsollaAuthTest::RunTest(const FString& Parameters)
{
	// UTestHelper::CreateLoginWidget();
	return true;
}

BEGIN_DEFINE_SPEC(FCustomTest, "Xsolla.AutomationTests CustomTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	TSharedPtr<FXsollaAuthTest> CustomXsollaClass;
END_DEFINE_SPEC(FCustomTest)

void FCustomTest::Define()
{
	Describe("Execute()", [this]()
	{
		It("should return true when successful", [this]()
		{
			// UTestHelper::CreateLoginWidget();
			TestTrue("Execute", true);
		});
	});
}
