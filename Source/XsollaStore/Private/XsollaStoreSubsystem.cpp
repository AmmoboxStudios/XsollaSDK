// Copyright 2021 Xsolla Inc. All Rights Reserved.

#include "XsollaStoreSubsystem.h"

#include "XsollaStore.h"
#include "XsollaStoreDataModel.h"
#include "XsollaStoreDefines.h"
#include "XsollaStoreSave.h"
#include "XsollaUtilsLibrary.h"
#include "XsollaUtilsTokenParser.h"
#include "XsollaUtilsUrlBuilder.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "JsonObjectConverter.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/ConstructorHelpers.h"
#include "XsollaOrderCheckObject.h"
#include "XsollaSettingsModule.h"
#include "XsollaProjectSettings.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "XsollaWebBrowserWrapper.h"
#include "XsollaLoginSubsystem.h"
#include "XsollaLoginLibrary.h"

#define LOCTEXT_NAMESPACE "FXsollaStoreModule"

UXsollaStoreSubsystem::UXsollaStoreSubsystem()
	: UGameInstanceSubsystem()
{
#if !UE_SERVER
	static ConstructorHelpers::FClassFinder<UUserWidget> BrowserWidgetFinder(*FString::Printf(TEXT("/%s/Store/Components/W_StoreBrowser.W_StoreBrowser_C"),
		*UXsollaUtilsLibrary::GetPluginName(FXsollaStoreModule::ModuleName)));
	DefaultBrowserWidgetClass = BrowserWidgetFinder.Class;
#endif
}

void UXsollaStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Initialize subsystem with project identifier provided by user
	const UXsollaProjectSettings* Settings = FXsollaSettingsModule::Get().GetSettings();
	Initialize(Settings->ProjectID);

	LoginSubsystem = GetGameInstance()->GetSubsystem<UXsollaLoginSubsystem>();
	
	UE_LOG(LogXsollaStore, Log, TEXT("%s: XsollaStore subsystem initialized"), *VA_FUNC_LINE);
}

void UXsollaStoreSubsystem::Deinitialize()
{
	// Do nothing for now
	Super::Deinitialize();
}

void UXsollaStoreSubsystem::Initialize(const FString& InProjectId)
{
	ProjectID = InProjectId;

	LoadData();
}

void UXsollaStoreSubsystem::GetVirtualItems(const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnStoreItemsUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_items"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetVirtualItems_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetItemGroups(const FString& Locale,
	const FOnItemGroupsUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/groups"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetItemGroups_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetVirtualCurrencies(const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnVirtualCurrenciesUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectId}/items/virtual_currency"))
							.SetPathParam(TEXT("ProjectId"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetVirtualCurrencies_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetVirtualCurrencyPackages(const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnVirtualCurrencyPackagesUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_currency/package"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetVirtualCurrencyPackages_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetItemsListBySpecifiedGroup(const FString& ExternalId,
	const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGetItemsListBySpecifiedGroup& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_items/group/{ExternalId}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ExternalId"), ExternalId.IsEmpty() ? TEXT("all") : ExternalId)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetItemsListBySpecifiedGroup_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetAllItemsList(const FString& Locale,
	const FOnGetItemsList& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_items/all"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetAllItemsList_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::FetchPaymentToken(const FString& AuthToken, const FString& ItemSKU,
	const FString& Currency, const FString& Country, const FString& Locale,
	const FXsollaParameters CustomParameters,
	const FOnFetchTokenSuccess& SuccessCallback, const FOnError& ErrorCallback, const int32 Quantity)
{
	TSharedPtr<FJsonObject> RequestDataJson = PreparePaymentTokenRequestPayload(Currency, Country, Locale, CustomParameters);

	RequestDataJson->SetNumberField(TEXT("quantity"), Quantity);

	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/payment/item/{ItemSKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ItemSKU"), ItemSKU)
							.Build();

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, RequestDataJson, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_POST, Token, SerializeJson(RequestDataJson));

		if (IOnlineSubsystem::IsEnabled(STEAM_SUBSYSTEM))
		{
			FString SteamId;
			FString OutError;

			if (!GetSteamUserId(Token, SteamId, OutError))
			{
				ErrorCallback.ExecuteIfBound(0, 0, OutError);
				return;
			}

			HttpRequest->SetHeader(TEXT("x-steam-userid"), SteamId);
		}
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::FetchPaymentToken_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::FetchCartPaymentToken(const FString& AuthToken, const FString& CartId,
	const FString& Currency, const FString& Country, const FString& Locale,
	const FXsollaParameters CustomParameters,
	const FOnFetchTokenSuccess& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;
	CachedCartId = CartId;

	TSharedPtr<FJsonObject> RequestDataJson = PreparePaymentTokenRequestPayload(Currency, Country, Locale, CustomParameters);

	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/payment/cart")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/payment/cart/{CartID}");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), Cart.cart_id)
							.Build();

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, RequestDataJson, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_POST, Token, SerializeJson(RequestDataJson));

		if (IOnlineSubsystem::IsEnabled(STEAM_SUBSYSTEM))
		{
			FString SteamId;
			FString OutError;

			if (!GetSteamUserId(AuthToken, SteamId, OutError))
			{
				ErrorCallback.ExecuteIfBound(0, 0, OutError);
				return;
			}

			HttpRequest->SetHeader(TEXT("x-steam-userid"), SteamId);
		}
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::FetchPaymentToken_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::LaunchPaymentConsole(UObject* WorldContextObject, const int32 OrderId, const FString& AccessToken,
	const FOnStoreSuccessPayment& SuccessCallback, const FOnError& ErrorCallback)
{
	FString PaystationUrl;
	if (IsSandboxEnabled())
	{
		PaystationUrl = FString::Printf(TEXT("https://sandbox-secure.xsolla.com/paystation3?access_token=%s"), *AccessToken);
	}
	else
	{
		PaystationUrl = FString::Printf(TEXT("https://secure.xsolla.com/paystation3?access_token=%s"), *AccessToken);
	}

	const UXsollaProjectSettings* Settings = FXsollaSettingsModule::Get().GetSettings();
	if (Settings->UsePlatformBrowser)
	{
		UE_LOG(LogXsollaStore, Log, TEXT("%s: Launching Paystation: %s"), *VA_FUNC_LINE, *PaystationUrl);

		FPlatformProcess::LaunchURL(*PaystationUrl, nullptr, nullptr);
		CheckPendingOrder(AccessToken, OrderId, SuccessCallback, ErrorCallback);
	}
	else
	{
		UE_LOG(LogXsollaStore, Log, TEXT("%s: Loading Paystation: %s"), *VA_FUNC_LINE, *PaystationUrl);

		PengindPaystationUrl = PaystationUrl;

		if (MyBrowser == nullptr || !MyBrowser->IsValidLowLevel() || !MyBrowser->GetIsEnabled())
		{
			MyBrowser = CreateWidget<UXsollaWebBrowserWrapper>(WorldContextObject->GetWorld(), DefaultBrowserWidgetClass);
			MyBrowser->AddToViewport(100000);
		}
		else
		{
			MyBrowser->SetVisibility(ESlateVisibility::Visible);
		}

		MyBrowser->OnBrowserClosed.Unbind();
		MyBrowser->OnBrowserClosed.BindLambda([&, AccessToken, OrderId, SuccessCallback, ErrorCallback]()
		{
			CheckPendingOrder(AccessToken, OrderId, SuccessCallback, ErrorCallback);
		});
	}
}

void UXsollaStoreSubsystem::CheckPendingOrder(const FString& AccessToken, const int32 OrderId,
	const FOnStoreSuccessPayment& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("wss://store-ws.xsolla.com/sub/order/status"))
							.AddStringQueryParam(TEXT("order_id"), FString::FromInt(OrderId)) //FString casting to prevent parameters reorder.
							.AddStringQueryParam(TEXT("project_id"), ProjectID)
							.Build();

	auto OrderCheckObject = NewObject<UXsollaOrderCheckObject>(this);

	FOnOrderCheckSuccess OrderCheckSuccessCallback;
	OrderCheckSuccessCallback.BindLambda([&, OrderCheckObject, SuccessCallback](int32 OrderId, EXsollaOrderStatus OrderStatus)
	{
		if (OrderStatus == EXsollaOrderStatus::Done)
		{
			SuccessCallback.ExecuteIfBound();
			OrderCheckObject->Destroy();
			CachedOrderCheckObjects.Remove(OrderCheckObject);
		}
	});

	FOnOrderCheckError OrderCheckErrorCallback;
	OrderCheckErrorCallback.BindLambda([&, OrderCheckObject, AccessToken, OrderId, SuccessCallback, ErrorCallback](const FString& ErrorMessage)
	{
		ShortPollingCheckOrder(AccessToken, OrderId, SuccessCallback, ErrorCallback);
		OrderCheckObject->Destroy();
		CachedOrderCheckObjects.Remove(OrderCheckObject);
	});

	FOnOrderCheckTimeout OrderCheckTimeoutCallback;
	OrderCheckTimeoutCallback.BindLambda([&, OrderCheckObject, AccessToken, OrderId, SuccessCallback, ErrorCallback]()
	{
		ShortPollingCheckOrder(AccessToken, OrderId, SuccessCallback, ErrorCallback);
		OrderCheckObject->Destroy();
		CachedOrderCheckObjects.Remove(OrderCheckObject);
	});
	
	OrderCheckObject->Init(Url, TEXT("wss"), OrderCheckSuccessCallback, OrderCheckErrorCallback, OrderCheckTimeoutCallback, 300);
	CachedOrderCheckObjects.Add(OrderCheckObject);
	OrderCheckObject->Connect();
}

void UXsollaStoreSubsystem::ShortPollingCheckOrder(const FString& AccessToken, const int32 OrderId,
	const FOnStoreSuccessPayment& SuccessCallback, const FOnError& ErrorCallback)
{
	FOnCheckOrder CheckOrderSuccessCallback;
	CheckOrderSuccessCallback.BindLambda([&, AccessToken, SuccessCallback, ErrorCallback](int32 OrderId, EXsollaOrderStatus OrderStatus, FXsollaOrderContent OrderContent)
	{
		if (OrderStatus == EXsollaOrderStatus::New || OrderStatus == EXsollaOrderStatus::Paid)
		{
			FTimerHandle TimerHandle;
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda([&, AccessToken, OrderId, SuccessCallback, ErrorCallback]()
			{
				ShortPollingCheckOrder(AccessToken, OrderId, SuccessCallback, ErrorCallback);
			});
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.0f, false);
		}

		if (OrderStatus == EXsollaOrderStatus::Done)
		{
			SuccessCallback.ExecuteIfBound();
		}
	});
	
	CheckOrder(AccessToken, OrderId, CheckOrderSuccessCallback, ErrorCallback);
}

bool UXsollaStoreSubsystem::CheckOrder(const FString& AuthToken, const int32 OrderId,
	const FOnCheckOrder& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;

	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/order/{OrderId}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("OrderId"), OrderId)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET, AuthToken);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::CheckOrder_HttpRequestComplete, SuccessCallback, ErrorCallback);
	return HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::ClearCart(const FString& AuthToken, const FString& CartId,
	const FOnStoreCartUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;
	CachedCartId = CartId;

	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/clear")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/{CartID}/clear");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), Cart.cart_id)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_PUT, AuthToken);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::ClearCart_HttpRequestComplete, SuccessCallback, ErrorCallback);

	CartRequestsQueue.Add(HttpRequest);
	ProcessNextCartRequest();

	// Just cleanup local cart
	Cart.Items.Empty();
	OnCartUpdate.Broadcast(Cart);
}

void UXsollaStoreSubsystem::UpdateCart(const FString& AuthToken, const FString& CartId,
	const FString& Currency, const FString& Locale,
	const FOnStoreCartUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;
	CachedCartId = CartId;
	CachedCartCurrency = Currency;
	CachedCartLocale = Locale;

	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/{CartID}");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), Cart.cart_id)
							.AddStringQueryParam(TEXT("currency"), Currency)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET, AuthToken);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::UpdateCart_HttpRequestComplete, SuccessCallback, ErrorCallback);

	CartRequestsQueue.Add(HttpRequest);
	ProcessNextCartRequest();
}

void UXsollaStoreSubsystem::AddToCart(const FString& AuthToken, const FString& CartId,
	const FString& ItemSKU, const int32 Quantity,
	const FOnStoreCartUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;
	CachedCartId = CartId;

	// Prepare request payload
	TSharedPtr<FJsonObject> RequestDataJson = MakeShareable(new FJsonObject);
	RequestDataJson->SetNumberField(TEXT("quantity"), Quantity);

	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/item/{ItemSKU}")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/{CartID}/item/{ItemSKU}");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), Cart.cart_id)
							.SetPathParam(TEXT("ItemSKU"), ItemSKU)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_PUT, AuthToken, SerializeJson(RequestDataJson));
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::AddToCart_HttpRequestComplete, SuccessCallback, ErrorCallback);

	CartRequestsQueue.Add(HttpRequest);
	ProcessNextCartRequest();

	// Try to update item quantity
	auto CartItem = Cart.Items.FindByPredicate([ItemSKU](const FStoreCartItem& InItem) {
		return InItem.sku == ItemSKU;
	});

	if (CartItem)
	{
		CartItem->quantity = FMath::Max(0, Quantity);
	}
	else
	{
		auto StoreItem = ItemsData.Items.FindByPredicate([ItemSKU](const FStoreItem& InItem) {
			return InItem.sku == ItemSKU;
		});

		if (StoreItem)
		{
			FStoreCartItem Item(*StoreItem);
			Item.quantity = FMath::Max(0, Quantity);
			Cart.Items.Add(Item);
		}
		else
		{
			auto CurrencyPackageItem = VirtualCurrencyPackages.Items.FindByPredicate([ItemSKU](const FVirtualCurrencyPackage& InItem) {
				return InItem.sku == ItemSKU;
			});

			if (CurrencyPackageItem)
			{
				FStoreCartItem Item(*CurrencyPackageItem);
				Item.quantity = FMath::Max(0, Quantity);

				Cart.Items.Add(Item);
			}
			else
			{
				UE_LOG(LogXsollaStore, Error, TEXT("%s: Can't find provided SKU in local cache: %s"), *VA_FUNC_LINE, *ItemSKU);
			}
		}
	}

	OnCartUpdate.Broadcast(Cart);
}

void UXsollaStoreSubsystem::RemoveFromCart(const FString& AuthToken, const FString& CartId, const FString& ItemSKU,
	const FOnStoreCartUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;
	CachedCartId = CartId;

	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/item/{ItemSKU}")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/{CartID}/item/{ItemSKU}");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), Cart.cart_id)
							.SetPathParam(TEXT("ItemSKU"), ItemSKU)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_DELETE, AuthToken);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::RemoveFromCart_HttpRequestComplete, SuccessCallback, ErrorCallback);

	CartRequestsQueue.Add(HttpRequest);
	ProcessNextCartRequest();

	for (int32 i = Cart.Items.Num() - 1; i >= 0; --i)
	{
		if (Cart.Items[i].sku == ItemSKU)
		{
			Cart.Items.RemoveAt(i);
			break;
		}
	}

	OnCartUpdate.Broadcast(Cart);
}

void UXsollaStoreSubsystem::FillCartById(const FString& AuthToken, const FString& CartId, const TArray<FStoreCartItem>& Items,
	const FOnStoreCartUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Endpoint = CartId.IsEmpty()
								 ? TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/fill")
								 : TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/cart/{CartID}/fill");

	const FString Url = XsollaUtilsUrlBuilder(Endpoint)
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CartID"), CartId)
							.Build();

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> JsonItems;

	for (auto& Item : Items)
	{
		TSharedPtr<FJsonObject> JsonItem = MakeShareable(new FJsonObject());
		JsonItem->SetStringField(TEXT("sku"), Item.sku);
		JsonItem->SetNumberField(TEXT("quantity"), Item.quantity);
		JsonItems.Push(MakeShareable(new FJsonValueObject(JsonItem)));
	}

	JsonObject->SetArrayField(TEXT("items"), JsonItems);

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, JsonObject, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_PUT, Token, SerializeJson(JsonObject));
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::FillCartById_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::GetSpecifiedBundle(const FString& Sku, const FOnGetSpecifiedBundleUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/bundle/sku/{Sku}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("Sku"), Sku)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetSpecifiedBundle_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetBundles(const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGetListOfBundlesUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/bundle"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetListOfBundles_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetVirtualCurrency(const FString& CurrencySKU,
	const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnCurrencyUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_currency/sku/{CurrencySKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("CurrencySKU"), CurrencySKU)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetVirtualCurrency_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetVirtualCurrencyPackage(const FString& PackageSKU,
	const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnCurrencyPackageUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/virtual_currency/package/sku/{PackageSKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("PackageSKU"), PackageSKU)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetVirtualCurrencyPackage_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::BuyItemWithVirtualCurrency(const FString& AuthToken,
	const FString& ItemSKU, const FString& CurrencySKU, const EXsollaPublishingPlatform Platform,
	const FOnPurchaseUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	CachedAuthToken = AuthToken;

	const FString PlatformName = Platform == EXsollaPublishingPlatform::undefined ? TEXT("") : UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPublishingPlatform", Platform);
	
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/payment/item/{ItemSKU}/virtual/{CurrencySKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ItemSKU"), ItemSKU)
							.SetPathParam(TEXT("CurrencySKU"), CurrencySKU)
							.AddStringQueryParam(TEXT("platform"), PlatformName)
							.Build();

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_POST, Token);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::BuyItemWithVirtualCurrency_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::GetPromocodeRewards(const FString& AuthToken, const FString& PromocodeCode,
	const FOnGetPromocodeRewardsUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/promocode/code/{PromocodeCode}/rewards"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("PromocodeCode"), PromocodeCode)
							.Build();

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET, Token);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::GetPromocodeRewards_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::RedeemPromocode(const FString& AuthToken, const FString& PromocodeCode,
	const FOnPromocodeUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/promocode/redeem"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.Build();

	// Prepare request payload
	TSharedPtr<FJsonObject> RequestDataJson = MakeShareable(new FJsonObject);
	RequestDataJson->SetStringField(TEXT("coupon_code"), PromocodeCode);

	TSharedPtr<FJsonObject> JsonCart = MakeShareable(new FJsonObject);
	JsonCart->SetStringField(TEXT("id"), Cart.cart_id);
	RequestDataJson->SetObjectField(TEXT("cart"), JsonCart);

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, RequestDataJson, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_POST, Token, SerializeJson(RequestDataJson));
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::RedeemPromocode_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::RemovePromocodeFromCart(const FString& AuthToken,
	const FOnPromocodeUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/promocode/remove"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.Build();

	// Prepare request payload
	TSharedPtr<FJsonObject> RequestDataJson = MakeShareable(new FJsonObject);

	TSharedPtr<FJsonObject> JsonCart = MakeShareable(new FJsonObject);
	JsonCart->SetStringField(TEXT("id"), Cart.cart_id);
	RequestDataJson->SetObjectField(TEXT("cart"), JsonCart);

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, RequestDataJson, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_PUT, Token, SerializeJson(RequestDataJson));
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::RemovePromocodeFromCart_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::GetGamesList(const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnStoreGamesUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetGamesList_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetGamesListBySpecifiedGroup(const FString& ExternalId, const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGetGamesListBySpecifiedGroup& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game/group/{ExternalId}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ExternalId"), ExternalId.IsEmpty() ? TEXT("all") : ExternalId)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetGamesListBySpecifiedGroup_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetGameItem(const FString& GameSKU, const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGameUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game/sku/{GameSKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("GameSKU"), GameSKU)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetGameItem_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetGameKeyItem(const FString& ItemSKU, const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGameKeyUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game/key/sku/{ItemSKU}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ItemSKU"), ItemSKU)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetGameKeyItem_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetGameKeysListBySpecifiedGroup(const FString& ExternalId, const FString& Locale, const FString& Country, const TArray<FString>& AdditionalFields,
	const FOnGetGameKeysListBySpecifiedGroup& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game/key/group/{ExternalId}"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.SetPathParam(TEXT("ExternalId"), ExternalId.IsEmpty() ? TEXT("all") : ExternalId)
							.AddStringQueryParam(TEXT("locale"), Locale)
							.AddStringQueryParam(TEXT("country"), Country)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetGameKeysListBySpecifiedGroup_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetDRMList(const FOnDRMListUpdate& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/items/game/drm"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.Build();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET);
	HttpRequest->OnProcessRequestComplete().BindUObject(this,
		&UXsollaStoreSubsystem::GetDRMList_HttpRequestComplete, SuccessCallback, ErrorCallback);
	HttpRequest->ProcessRequest();
}

void UXsollaStoreSubsystem::GetOwnedGames(const FString& AuthToken, const TArray<FString>& AdditionalFields,
	const FOnOwnedGamesListUpdate& SuccessCallback, const FOnError& ErrorCallback, const int Limit, const int Offset, const bool bIsSandbox)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/entitlement"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.AddArrayQueryParam(TEXT("additional_fields[]"), AdditionalFields)
							.AddNumberQueryParam(TEXT("limit"), Limit)
							.AddNumberQueryParam(TEXT("offset"), Offset)
							.AddBoolQueryParam(TEXT("sandbox"), bIsSandbox, true)
							.Build();

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_GET, Token);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::GetOwnedGames_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::RedeemGameCodeByClient(const FString& AuthToken, const FString& Code,
	const FOnRedeemGameCodeSuccess& SuccessCallback, const FOnError& ErrorCallback)
{
	const FString Url = XsollaUtilsUrlBuilder(TEXT("https://store.xsolla.com/api/v2/project/{ProjectID}/entitlement/redeem"))
							.SetPathParam(TEXT("ProjectID"), ProjectID)
							.Build();

	// Prepare request payload
	TSharedPtr<FJsonObject> RequestDataJson = MakeShareable(new FJsonObject);
	RequestDataJson->SetStringField(TEXT("code"), Code);
	RequestDataJson->SetBoolField(TEXT("sandbox"), IsSandboxEnabled());

	FOnTokenUpdate SuccessTokenUpdate;
	SuccessTokenUpdate.BindLambda([&, Url, RequestDataJson, SuccessCallback, ErrorCallback, SuccessTokenUpdate](const FString& Token, bool bRepeatOnError)
	{
		const auto ErrorHandlersWrapper = FErrorHandlersWrapper(bRepeatOnError, SuccessTokenUpdate, ErrorCallback);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(Url, EXsollaHttpRequestVerb::VERB_POST, Token, SerializeJson(RequestDataJson));
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UXsollaStoreSubsystem::RedeemGameCodeByClient_HttpRequestComplete, SuccessCallback, ErrorHandlersWrapper);
		HttpRequest->ProcessRequest();
	});

	SuccessTokenUpdate.ExecuteIfBound(AuthToken, true);
}

void UXsollaStoreSubsystem::GetVirtualItems_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreItemsUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreItemsData::StaticStruct(), &ItemsData, OutError))
	{
		// Update categories
		for (const auto& Item : ItemsData.Items)
		{
			for (const auto& ItemGroup : Item.groups)
			{
				ItemsData.GroupIds.Add(ItemGroup.external_id);
			}
		}

		SuccessCallback.ExecuteIfBound(ItemsData);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetItemGroups_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnItemGroupsUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreItemsData GroupsData;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreItemsData::StaticStruct(), &GroupsData, OutError))
	{
		ItemsData.Groups = GroupsData.Groups;
		SuccessCallback.ExecuteIfBound(GroupsData.Groups);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetVirtualCurrencies_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnVirtualCurrenciesUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FVirtualCurrencyData VirtualCurrencyData;
	
	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FVirtualCurrencyData::StaticStruct(), &VirtualCurrencyData, OutError))
	{
		SuccessCallback.ExecuteIfBound(VirtualCurrencyData);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetVirtualCurrencyPackages_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnVirtualCurrencyPackagesUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FVirtualCurrencyPackagesData::StaticStruct(), &VirtualCurrencyPackages, OutError))
	{
		SuccessCallback.ExecuteIfBound(VirtualCurrencyPackages);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetItemsListBySpecifiedGroup_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetItemsListBySpecifiedGroup SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreItemsList Items;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreItemsList::StaticStruct(), &Items, OutError))
	{
		SuccessCallback.ExecuteIfBound(Items);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetAllItemsList_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetItemsList SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreItemsList Items;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreItemsList::StaticStruct(), &Items, OutError))
	{
		SuccessCallback.ExecuteIfBound(Items);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::FetchPaymentToken_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnFetchTokenSuccess SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	TSharedPtr<FJsonObject> JsonObject;
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsJson(HttpRequest, HttpResponse, bSucceeded, JsonObject, OutError))
	{
		FString AccessToken = JsonObject->GetStringField(TEXT("token"));
		int32 OrderId = JsonObject->GetNumberField(TEXT("order_id"));

		SuccessCallback.ExecuteIfBound(AccessToken, OrderId);
		return;
	}

	LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
}

void UXsollaStoreSubsystem::CheckOrder_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnCheckOrder SuccessCallback, FOnError ErrorCallback)
{
	FXsollaOrder Order;
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FXsollaOrder::StaticStruct(), &Order, OutError))
	{
		FString OrderStatusStr = Order.status;

		EXsollaOrderStatus OrderStatus = EXsollaOrderStatus::Unknown;

		if (OrderStatusStr == TEXT("new"))
		{
			OrderStatus = EXsollaOrderStatus::New;
		}
		else if (OrderStatusStr == TEXT("paid"))
		{
			OrderStatus = EXsollaOrderStatus::Paid;
		}
		else if (OrderStatusStr == TEXT("done"))
		{
			OrderStatus = EXsollaOrderStatus::Done;
		}
		else if (OrderStatusStr == TEXT("canceled"))
		{
			OrderStatus = EXsollaOrderStatus::Canceled;
		}
		else
		{
			UE_LOG(LogXsollaStore, Warning, TEXT("%s: Unknown order status: %s [%d]"), *VA_FUNC_LINE, *OrderStatusStr, Order.order_id);
		}

		SuccessCallback.ExecuteIfBound(Order.order_id, OrderStatus, Order.content);
		return;
	}

	HandleRequestError(OutError, ErrorCallback);
}

void UXsollaStoreSubsystem::CreateCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FOnError ErrorCallback)
{
	TSharedPtr<FJsonObject> JsonObject;
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsJson(HttpRequest, HttpResponse, bSucceeded, JsonObject, OutError))
	{
		Cart = FStoreCart(JsonObject->GetStringField(TEXT("cart_id")));
		OnCartUpdate.Broadcast(Cart);

		SaveData();

		SuccessCallback.ExecuteIfBound();
		return;
	}

	ProcessNextCartRequest();
	HandleRequestError(OutError, ErrorCallback);
}

void UXsollaStoreSubsystem::ClearCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponse(HttpRequest, HttpResponse, bSucceeded, OutError))
	{
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}

	ProcessNextCartRequest();
}

void UXsollaStoreSubsystem::UpdateCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreCart::StaticStruct(), &Cart, OutError))
	{
		OnCartUpdate.Broadcast(Cart);
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}

	ProcessNextCartRequest();
}

void UXsollaStoreSubsystem::AddToCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponse(HttpRequest, HttpResponse, bSucceeded, OutError))
	{
		ProcessNextCartRequest();
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		UpdateCart(CachedAuthToken, CachedCartId, CachedCartCurrency, CachedCartLocale, SuccessCallback, ErrorCallback);
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::RemoveFromCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponse(HttpRequest, HttpResponse, bSucceeded, OutError))
	{
		ProcessNextCartRequest();
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		UpdateCart(CachedAuthToken, CachedCartId, CachedCartCurrency, CachedCartLocale, SuccessCallback, ErrorCallback);
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::FillCartById_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreCartUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreCart::StaticStruct(), &Cart, OutError))
	{
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::GetListOfBundles_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetListOfBundlesUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreListOfBundles ListOfBundles;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreListOfBundles::StaticStruct(), &ListOfBundles, OutError))
	{
		for (const auto& Bundle : ListOfBundles.items)
		{
			ItemsData.Items.Add(Bundle);
		}

		for (const auto& Bundle : ListOfBundles.items)
		{
			for (const auto& BundleGroup : Bundle.groups)
			{
				ItemsData.GroupIds.Add(BundleGroup.external_id);
			}
		}

		SuccessCallback.ExecuteIfBound(ListOfBundles);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetSpecifiedBundle_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetSpecifiedBundleUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreBundle Bundle;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreBundle::StaticStruct(), &Bundle, OutError))
	{
		SuccessCallback.ExecuteIfBound(Bundle);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetVirtualCurrency_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnCurrencyUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FVirtualCurrency currency;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FVirtualCurrency::StaticStruct(), &currency, OutError))
	{
		SuccessCallback.ExecuteIfBound(currency);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetVirtualCurrencyPackage_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnCurrencyPackageUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FVirtualCurrencyPackage currencyPackage;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FVirtualCurrencyPackage::StaticStruct(), &currencyPackage, OutError))
	{
		SuccessCallback.ExecuteIfBound(currencyPackage);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::BuyItemWithVirtualCurrency_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnPurchaseUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	TSharedPtr<FJsonObject> JsonObject;
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsJson(HttpRequest, HttpResponse, bSucceeded, JsonObject, OutError))
	{
		int32 OrderId = JsonObject->GetNumberField(TEXT("order_id"));
		SuccessCallback.ExecuteIfBound(OrderId);
		return;
	}

	LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
}

void UXsollaStoreSubsystem::GetPromocodeRewards_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetPromocodeRewardsUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;
	FStorePromocodeRewardData PromocodeRewardData;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStorePromocodeRewardData::StaticStruct(), &PromocodeRewardData, OutError))
	{
		SuccessCallback.ExecuteIfBound(PromocodeRewardData);
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::RedeemPromocode_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnPromocodeUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreCart::StaticStruct(), &Cart, OutError))
	{
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::RemovePromocodeFromCart_HttpRequestComplete(
	FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnPromocodeUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreCart::StaticStruct(), &Cart, OutError))
	{
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::GetGamesList_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnStoreGamesUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreGamesData GamesData;
	
	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreGamesData::StaticStruct(), &GamesData, OutError))
	{
		// Update categories
		for (const auto& Item : GamesData.Items)
		{
			for (const auto& ItemGroup : Item.groups)
			{
				GamesData.GroupIds.Add(ItemGroup.external_id);
			}
		}

		SuccessCallback.ExecuteIfBound(GamesData);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetGamesListBySpecifiedGroup_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetGamesListBySpecifiedGroup SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreGamesList Games;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreGamesList::StaticStruct(), &Games, OutError))
	{
		SuccessCallback.ExecuteIfBound(Games);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetGameItem_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGameUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FGameItem Game;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FGameItem::StaticStruct(), &Game, OutError))
	{
		SuccessCallback.ExecuteIfBound(Game);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetGameKeyItem_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGameKeyUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FGameKeyItem GameKey;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FGameKeyItem::StaticStruct(), &GameKey, OutError))
	{
		SuccessCallback.ExecuteIfBound(GameKey);
	}
	else
	{
		ProcessNextCartRequest();
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetGameKeysListBySpecifiedGroup_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnGetGameKeysListBySpecifiedGroup SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreGameKeysList GameKeys;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreGameKeysList::StaticStruct(), &GameKeys, OutError))
	{
		SuccessCallback.ExecuteIfBound(GameKeys);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetDRMList_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnDRMListUpdate SuccessCallback, FOnError ErrorCallback)
{
	XsollaHttpRequestError OutError;
	FStoreDRMList DRMList;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FStoreDRMList::StaticStruct(), &DRMList, OutError))
	{
		SuccessCallback.ExecuteIfBound(DRMList);
	}
	else
	{
		HandleRequestError(OutError, ErrorCallback);
	}
}

void UXsollaStoreSubsystem::GetOwnedGames_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnOwnedGamesListUpdate SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;
	FOwnedGamesList OwnedGamesList;

	if (XsollaUtilsHttpRequestHelper::ParseResponseAsStruct(HttpRequest, HttpResponse, bSucceeded, FOwnedGamesList::StaticStruct(), &OwnedGamesList, OutError))
	{
		SuccessCallback.ExecuteIfBound(OwnedGamesList);
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::RedeemGameCodeByClient_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
	const bool bSucceeded, FOnRedeemGameCodeSuccess SuccessCallback, FErrorHandlersWrapper ErrorHandlersWrapper)
{
	XsollaHttpRequestError OutError;

	if (XsollaUtilsHttpRequestHelper::ParseResponse(HttpRequest, HttpResponse, bSucceeded, OutError))
	{
		SuccessCallback.ExecuteIfBound();
	}
	else
	{
		LoginSubsystem->HandleRequestError(OutError, ErrorHandlersWrapper);
	}
}

void UXsollaStoreSubsystem::HandleRequestError(XsollaHttpRequestError ErrorData, FOnError ErrorCallback)
{
	auto errorMessage = ErrorData.errorMessage.IsEmpty() ? ErrorData.description : ErrorData.errorMessage;
	UE_LOG(LogXsollaStore, Error, TEXT("%s: request failed - Status code: %d, Error code: %d, Error message: %s"),
		*VA_FUNC_LINE, ErrorData.statusCode, ErrorData.errorCode, *errorMessage);
	ErrorCallback.ExecuteIfBound(ErrorData.statusCode, ErrorData.errorCode, errorMessage);
}

void UXsollaStoreSubsystem::LoadData()
{
	auto CartData = UXsollaStoreSave::Load();

	CachedCartCurrency = CartData.CartCurrency;
	Cart.cart_id = CartData.CartId;

	OnCartUpdate.Broadcast(Cart);
}

void UXsollaStoreSubsystem::SaveData()
{
	UXsollaStoreSave::Save(FXsollaStoreSaveData(Cart.cart_id, CachedCartCurrency));
}

bool UXsollaStoreSubsystem::IsSandboxEnabled() const
{
	const UXsollaProjectSettings* Settings = FXsollaSettingsModule::Get().GetSettings();
	bool bIsSandboxEnabled = Settings->EnableSandbox;

#if UE_BUILD_SHIPPING
	if (bIsSandboxEnabled)
	{
		UE_LOG(LogXsollaStore, Warning, TEXT("%s: Sandbox should be disabled in Shipping build"), *VA_FUNC_LINE);
	}
#endif // UE_BUILD_SHIPPING

	return bIsSandboxEnabled;
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UXsollaStoreSubsystem::CreateHttpRequest(const FString& Url, const EXsollaHttpRequestVerb Verb,
	const FString& AuthToken, const FString& Content)
{
	return XsollaUtilsHttpRequestHelper::CreateHttpRequest(Url, Verb, AuthToken, Content, TEXT("STORE"), XSOLLA_STORE_VERSION);
}

FString UXsollaStoreSubsystem::SerializeJson(const TSharedPtr<FJsonObject> DataJson) const
{
	FString JsonContent;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonContent);
	FJsonSerializer::Serialize(DataJson.ToSharedRef(), Writer);
	return JsonContent;
}

void UXsollaStoreSubsystem::ProcessNextCartRequest()
{
	// Cleanup finished requests firts
	int32 CartRequestsNum = CartRequestsQueue.Num();
	for (int32 i = CartRequestsNum - 1; i >= 0; --i)
	{
		if (CartRequestsQueue[i].Get().GetStatus() == EHttpRequestStatus::Succeeded ||
			CartRequestsQueue[i].Get().GetStatus() == EHttpRequestStatus::Failed ||
			CartRequestsQueue[i].Get().GetStatus() == EHttpRequestStatus::Failed_ConnectionError)
		{
			CartRequestsQueue.RemoveAt(i);
		}
	}

	// Check we have request in progress
	bool bRequestInProcess = false;
	for (int32 i = 0; i < CartRequestsQueue.Num(); ++i)
	{
		if (CartRequestsQueue[i].Get().GetStatus() == EHttpRequestStatus::Processing)
		{
			bRequestInProcess = true;
		}
	}

	// Launch next one if we have it
	if (!bRequestInProcess && CartRequestsQueue.Num() > 0)
	{
		CartRequestsQueue[0].Get().ProcessRequest();
	}
}

TSharedPtr<FJsonObject> UXsollaStoreSubsystem::PreparePaymentTokenRequestPayload(
	const FString& Currency, const FString& Country, const FString& Locale, const FXsollaParameters& CustomParameters)
{
	TSharedPtr<FJsonObject> RequestDataJson = MakeShareable(new FJsonObject);

	const UXsollaProjectSettings* Settings = FXsollaSettingsModule::Get().GetSettings();

	// General
	if (!Currency.IsEmpty())
		RequestDataJson->SetStringField(TEXT("currency"), Currency);
	if (!Country.IsEmpty())
		RequestDataJson->SetStringField(TEXT("country"), Country);
	if (!Locale.IsEmpty())
		RequestDataJson->SetStringField(TEXT("locale"), Locale);

	// Sandbox
	RequestDataJson->SetBoolField(TEXT("sandbox"), IsSandboxEnabled());

	// Custom parameters
	UXsollaUtilsLibrary::AddParametersToJsonObjectByFieldName(RequestDataJson, "custom_parameters", CustomParameters);

	// PayStation settings
	TSharedPtr<FJsonObject> PaymentSettingsJson = MakeShareable(new FJsonObject);

	TSharedPtr<FJsonObject> PaymentUiSettingsJson = MakeShareable(new FJsonObject);

	PaymentUiSettingsJson->SetStringField(TEXT("theme"), GetPaymentInerfaceTheme());
	PaymentUiSettingsJson->SetStringField(TEXT("size"),
		UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentUiSize", Settings->PaymentInterfaceSize));

	if (Settings->PaymentInterfaceVersion != EXsollaPaymentUiVersion::not_specified)
		PaymentUiSettingsJson->SetStringField(TEXT("version"),
			UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentUiVersion", Settings->PaymentInterfaceVersion));

	PaymentSettingsJson->SetObjectField(TEXT("ui"), PaymentUiSettingsJson);

	if (!Settings->UseSettingsFromPublisherAccount)
	{
		if (!Settings->ReturnUrl.IsEmpty())
			PaymentSettingsJson->SetStringField(TEXT("return_url"), Settings->ReturnUrl);

		TSharedPtr<FJsonObject> RedirectSettingsJson = MakeShareable(new FJsonObject);

		RedirectSettingsJson->SetStringField(TEXT("redirect_conditions"),
			UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentRedirectCondition", Settings->RedirectCondition));
		RedirectSettingsJson->SetStringField(TEXT("status_for_manual_redirection"),
			UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentRedirectStatusManual", Settings->RedirectStatusManual));

		RedirectSettingsJson->SetNumberField(TEXT("delay"), Settings->RedirectDelay);
		RedirectSettingsJson->SetStringField(TEXT("redirect_button_caption"), Settings->RedirectButtonCaption);

		PaymentSettingsJson->SetObjectField(TEXT("redirect_policy"), RedirectSettingsJson);
	}
	else
	{
		if (Settings->UsePlatformBrowser)
		{
#if PLATFORM_ANDROID||PLATFORM_IOS
			PaymentSettingsJson->SetStringField(TEXT("return_url"), FString::Printf(TEXT("app://xpayment.%s"), *UXsollaLoginLibrary::GetAppId()));
			TSharedPtr<FJsonObject> RedirectSettingsJson = MakeShareable(new FJsonObject);

			RedirectSettingsJson->SetStringField(TEXT("redirect_conditions"),
				UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentRedirectCondition", EXsollaPaymentRedirectCondition::any));
			RedirectSettingsJson->SetStringField(TEXT("status_for_manual_redirection"),
				UXsollaUtilsLibrary::GetEnumValueAsString("EXsollaPaymentRedirectStatusManual", EXsollaPaymentRedirectStatusManual::none));

			RedirectSettingsJson->SetNumberField(TEXT("delay"), 0);
			RedirectSettingsJson->SetStringField(TEXT("redirect_button_caption"), TEXT(""));

			PaymentSettingsJson->SetObjectField(TEXT("redirect_policy"), RedirectSettingsJson);
#endif	
		}
	}

	RequestDataJson->SetObjectField(TEXT("settings"), PaymentSettingsJson);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestDataJson.ToSharedRef(), Writer);
	
	UE_LOG(LogXsollaStore, Log, TEXT("%s: payment payload"), *OutputString);
	
	return RequestDataJson;
}

FString UXsollaStoreSubsystem::GetPaymentInerfaceTheme() const
{
	FString theme;

	const UXsollaProjectSettings* Settings = FXsollaSettingsModule::Get().GetSettings();

	switch (Settings->PaymentInterfaceTheme)
	{
	case EXsollaPaymentUiTheme::default_light:
		theme = TEXT("default");
		break;
	case EXsollaPaymentUiTheme::default_dark:
		theme = TEXT("default_dark");
		break;
	case EXsollaPaymentUiTheme::dark:
		theme = TEXT("dark");
		break;
	case EXsollaPaymentUiTheme::ps4_default_light:
		theme = TEXT("ps4-default-light");
		break;
	case EXsollaPaymentUiTheme::ps4_default_dark:
		theme = TEXT("ps4-default-dark");
		break;
	default:
		theme = TEXT("default");
	}

	return theme;
}

bool UXsollaStoreSubsystem::GetSteamUserId(const FString& AuthToken, FString& SteamId, FString& OutError)
{
	FString SteamIdUrl;
	if (!UXsollaUtilsTokenParser::GetStringTokenParam(AuthToken, TEXT("id"), SteamIdUrl))
	{
		OutError = TEXT("Can't parse token payload or can't find id in token payload");
		return false;
	}

	// Extract ID value from user's Steam profile URL
	int SteamIdIndex;
	if (SteamIdUrl.FindLastChar('/', SteamIdIndex))
	{
		SteamId = SteamIdUrl.RightChop(SteamIdIndex + 1);
	}

	return true;
}

TArray<FStoreItem> UXsollaStoreSubsystem::GetVirtualItemsWithoutGroup() const
{
	return ItemsData.Items.FilterByPredicate([](const FStoreItem& InStoreItem) {
		return InStoreItem.groups.Num() == 0;
	});
}

const FStoreItemsData& UXsollaStoreSubsystem::GetItemsData() const
{
	return ItemsData;
}

const FStoreCart& UXsollaStoreSubsystem::GetCart() const
{
	return Cart;
}

const FString& UXsollaStoreSubsystem::GetPendingPaystationUrl() const
{
	return PengindPaystationUrl;
}

FString UXsollaStoreSubsystem::GetItemName(const FString& ItemSKU) const
{
	auto StoreItem = ItemsData.Items.FindByPredicate([ItemSKU](const FStoreItem& InItem) {
		return InItem.sku == ItemSKU;
	});

	if (StoreItem != nullptr)
	{
		return StoreItem->name;
	}

	return TEXT("");
}

const FStoreItem& UXsollaStoreSubsystem::FindItemBySku(const FString& ItemSku, bool& bHasFound) const
{
	bHasFound = false;

	const auto StoreItem = ItemsData.Items.FindByPredicate([ItemSku](const FStoreItem& InItem) {
		return InItem.sku == ItemSku;
	});

	if (StoreItem != nullptr)
	{
		bHasFound = true;
		return StoreItem[0];
	}

	static const FStoreItem DefaultItem;
	return DefaultItem;
}

const FVirtualCurrencyPackage& UXsollaStoreSubsystem::FindVirtualCurrencyPackageBySku(const FString& ItemSku, bool& bHasFound) const
{
	bHasFound = false;

	const auto PackageItem = VirtualCurrencyPackages.Items.FindByPredicate([ItemSku](const FVirtualCurrencyPackage& InItem) {
		return InItem.sku == ItemSku;
	});

	if (PackageItem != nullptr)
	{
		bHasFound = true;
		return PackageItem[0];
	}

	static const FVirtualCurrencyPackage DefaultPackage;
	return DefaultPackage;
}

bool UXsollaStoreSubsystem::IsItemInCart(const FString& ItemSKU) const
{
	auto CartItem = Cart.Items.FindByPredicate([ItemSKU](const FStoreCartItem& InItem) {
		return InItem.sku == ItemSKU;
	});

	return CartItem != nullptr;
}

#undef LOCTEXT_NAMESPACE
