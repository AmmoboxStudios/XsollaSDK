// Copyright 2022 Xsolla Inc. All Rights Reserved.

#include "XsollaProjectSettings.h"

UXsollaProjectSettings::UXsollaProjectSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AccountLinkingURL = TEXT("https://livedemo.xsolla.com/sdk/sdk-shadow-account/link");
	PlatformAuthenticationURL = TEXT("https://livedemo.xsolla.com/sdk/sdk-shadow-account/auth");
	UseCrossPlatformAccountLinking = false;
	ProjectID = TEXT("77640");
	LoginID = TEXT("026201e3-7e40-11ea-a85b-42010aa80004");
	ClientID = TEXT("57");
	EncryptCachedCredentials = false;
	SendPasswordlessAuthURL = false;
	RedirectURI = TEXT("https://login.xsolla.com/api/blank");
	Platform = EXsollaPublishingPlatform::playstation_network;
	bAllowNativeAuth = false;
	PasswordlessAuthURL = TEXT("https://login-widget.xsolla.com/latest/confirm-status");
	WebStoreURL = TEXT("https://sitebuilder.xsolla.com/game/sdk-web-store/");
	EnableSandbox = true;
	UsePlatformBrowser = false;
	PaymentInterfaceTheme = EXsollaPaymentUiTheme::ps4_default_dark;
	PaymentInterfaceSize = EXsollaPaymentUiSize::medium;
	PaymentInterfaceVersion = EXsollaPaymentUiVersion::not_specified;
	UseSettingsFromPublisherAccount = true;
	ReturnUrl = TEXT("");
	RedirectCondition = EXsollaPaymentRedirectCondition::none;
	RedirectDelay = 0;
	RedirectStatusManual = EXsollaPaymentRedirectStatusManual::none;
	RedirectButtonCaption = TEXT("");
}
