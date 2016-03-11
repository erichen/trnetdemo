// Fill out your copyright notice in the Description page of Project Settings.

#include "trnetdemo.h"
#include "trnetDemoGameInstance.h"
#include "OnlineSubsystemUtils.h"

UtrnetDemoGameInstance::UtrnetDemoGameInstance(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
        , isLoading_(false)
		, hasExternalLoginUI_(false)
		, isLogin_(true){
	/** Bind function for CREATING a Session */
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UtrnetDemoGameInstance::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UtrnetDemoGameInstance::OnStartOnlineGameComplete);
	/** Bind function for FINDING a Session */
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UtrnetDemoGameInstance::OnFindSessionsComplete);
	/** Bind function for JOINING a Session */
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UtrnetDemoGameInstance::OnJoinSessionComplete);
	/** Bind function for DESTROYING a Session */
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UtrnetDemoGameInstance::OnDestroySessionComplete);

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	IOnlineExternalUIPtr externalUI = OnlineSub->GetExternalUIInterface();
	hasExternalLoginUI_ = externalUI.IsValid();
	UE_LOG(LogInit, Warning, TEXT("hasExternalLoginUI_: %s"), hasExternalLoginUI_ ? TEXT("true"):TEXT("false"));

	if (hasExternalLoginUI_) {
		isLogin_ = false;
	}
}

bool UtrnetDemoGameInstance::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
    // Get the Session Interface, so we can call the "CreateSession" function on it
    IOnlineSessionPtr Sessions = GetSession();

    if (!Sessions.IsValid() || !UserId.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("No Sessions or UserId found!"));
        return false;
   }
    /*
    Fill in all the Session Settings that we want to use.

    There are more with SessionSettings.Set(...);
    For example the Map or the GameMode/Type.
    */
    SessionSettings = MakeShareable(new FOnlineSessionSettings());

    SessionSettings->bIsLANMatch = bIsLAN;
    SessionSettings->bUsesPresence = bIsPresence;
    SessionSettings->NumPublicConnections = MaxNumPlayers;
    SessionSettings->NumPrivateConnections = 0;
    SessionSettings->bAllowInvites = true;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bAllowJoinViaPresence = true;
    SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

    SessionSettings->Set(SETTING_MAPNAME, FString("NewMap"), EOnlineDataAdvertisementType::ViaOnlineService);

    // Set the delegate to the Handle of the SessionInterface
    OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

    // Our delegate should get called when this is complete (doesn't need to be successful!)
    bool bCreateSession = Sessions->CreateSession(*UserId, SessionName, *SessionSettings);
    if (!bCreateSession) {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Create Sessions fail"));
    }
    return bCreateSession;
}

void UtrnetDemoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnCreateSessionComplete %s, %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("sucess") : TEXT("fail")));
    
    // Get the Session Interface to call the StartSession function
    IOnlineSessionPtr Sessions = GetSession();

    if (!Sessions.IsValid())
    {
        return;
    }
    // Clear the SessionComplete delegate handle, since we finished this call
    Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
    if (bWasSuccessful)
    {
        // Set the StartSession delegate handle
        OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);

        // Our StartSessionComplete delegate should get called after this
        Sessions->StartSession(SessionName);
    }
}

void UtrnetDemoGameInstance::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnStartSessionComplete %s, %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("sucess"): TEXT("fail")));

    // Get the Session Interface to clear the Delegate
    IOnlineSessionPtr Sessions = GetSession();
    if (Sessions.IsValid())
    {
        // Clear the delegate, since we are done with this call
        Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
    }

	// If the start was successful, we can open a NewMap if we want. Make sure to use "listen" as a parameter!
	if (bWasSuccessful)
	{
		UGameplayStatics::OpenLevel(GetWorld(), "ThirdPersonExampleMap", true, "listen");
	}
}

void UtrnetDemoGameInstance::FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence)
{
    isLoading_ = true;
    
    // Get the SessionInterface from our OnlineSubsystem
    IOnlineSessionPtr Sessions = GetSession();

    if (Sessions.IsValid() && Sessions.IsValid() && UserId.IsValid())
    {
        /*
        Fill in all the SearchSettings, like if we are searching for a LAN game and how many results we want to have!
        */
        SessionSearch = MakeShareable(new FOnlineSessionSearch());

        SessionSearch->bIsLanQuery = bIsLAN;
        SessionSearch->MaxSearchResults = 20;
        SessionSearch->PingBucketSize = 50;

        // We only want to set this Query Setting if "bIsPresence" is true
        if (bIsPresence)
        {
            SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bIsPresence, EOnlineComparisonOp::Equals);
        }

        TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();

        // Set the Delegate to the Delegate Handle of the FindSession function
        OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

        // Finally call the SessionInterface function. The Delegate gets called once this is finished
        if(!Sessions->FindSessions(*UserId, SearchSettingsRef)) {
            OnFindSessionsComplete(false);
        }
    } else {
        // If something goes wrong, just call the Delegate Function directly with "false".
        OnFindSessionsComplete(false);        
    }
}

void UtrnetDemoGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    isLoading_ = false;
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OFindSessionsComplete bSuccess: %d"), bWasSuccessful));

    // Get SessionInterface of the OnlineSubsystem
    IOnlineSessionPtr Sessions = GetSession();
    
    if (!Sessions.IsValid())
    {
        return;
    }
    // Clear the Delegate handle, since we finished this call
    Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

    // Just debugging the Number of Search results. Can be displayed in UMG or something later on
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Num Search Results: %d"), SessionSearch->SearchResults.Num()));

    // If we have found at least 1 session, we just going to debug them. You could add them to a list of UMG Widgets, like it is done in the BP version!
    if (SessionSearch->SearchResults.Num() > 0)
    {
        // "SessionSearch->SearchResults" is an Array that contains all the information. You can access the Session in this and get a lot of information.
        // This can be customized later on with your own classes to add more information that can be set and displayed
        for (int32 SearchIdx = 0; SearchIdx < SessionSearch->SearchResults.Num(); SearchIdx++)
        {
            // OwningUserName is just the SessionName for now. I guess you can create your own Host Settings class and GameSession Class and add a proper GameServer Name here.
            // This is something you can't do in Blueprint for example!
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Session Number: %d | Sessionname: %s "), SearchIdx + 1, *(SessionSearch->SearchResults[SearchIdx].Session.OwningUserName)));
        }
    }
}

bool UtrnetDemoGameInstance::JoinSessionA(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
    // Get SessionInterface from the OnlineSubsystem
    IOnlineSessionPtr Sessions = GetSession();

    if (!Sessions.IsValid() || !UserId.IsValid())
    {
        return false;
    }
    // Set the Handle again
    OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

    // Call the "JoinSession" Function with the passed "SearchResult". The "SessionSearch->SearchResults" can be used to get such a
    // "FOnlineSessionSearchResult" and pass it. Pretty straight forward!
    isLoading_ = true;
    return Sessions->JoinSession(*UserId, SessionName, SearchResult);
}

void UtrnetDemoGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    isLoading_ = false;
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnJoinSessionComplete %s, %d"), *SessionName.ToString(), static_cast<int32>(Result)));

    // Get SessionInterface from the OnlineSubsystem
    IOnlineSessionPtr Sessions = GetSession();

    if (!Sessions.IsValid())
    {
        return;
    }
    // Clear the Delegate again
    Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

    // Get the first local PlayerController, so we can call "ClientTravel" to get to the Server Map
    // This is something the Blueprint Node "Join Session" does automatically!
    APlayerController * const PlayerController = GetFirstLocalPlayerController();

    // We need a FString to use ClientTravel and we can let the SessionInterface contruct such a
    // String for us by giving him the SessionName and an empty String. We want to do this, because
    // Every OnlineSubsystem uses different TravelURLs
    FString TravelURL;

    if (PlayerController && Sessions->GetResolvedConnectString(SessionName, TravelURL))
    {
        // Finally call the ClienTravel. If you want, you could print the TravelURL to see
        // how it really looks like
        PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
    }
}


void UtrnetDemoGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnDestroySessionComplete %s, %d"), *SessionName.ToString(), bWasSuccessful));

    // Get the SessionInterface from the OnlineSubsystem
    IOnlineSessionPtr Sessions = GetSession();

    if (!Sessions.IsValid())
    {
        return;
    }
    // Clear the Delegate
    Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);

    // If it was successful, we just load another level (could be a MainMenu!)
    if (bWasSuccessful)
    {
        UGameplayStatics::OpenLevel(GetWorld(), "Startup", true);
    }
}


void UtrnetDemoGameInstance::StartOnlineGame()
{
	// Creating a local player where we can get the UserID from
	ULocalPlayer* const Player = GetFirstGamePlayer();

	// Call our custom HostSession function. GameSessionName is a GameInstance variable
	HostSession(Player->GetPreferredUniqueNetId(), GameSessionName, true, true, 4);
}

void UtrnetDemoGameInstance::FindOnlineGames()
{
	ULocalPlayer* const Player = GetFirstGamePlayer();

	FindSessions(Player->GetPreferredUniqueNetId(), GameSessionName, true, true);
}

void UtrnetDemoGameInstance::JoinOnlineGame()
{
	ULocalPlayer* const Player = GetFirstGamePlayer();

	// Just a SearchResult where we can save the one we want to use, for the case we find more than one!
	FOnlineSessionSearchResult SearchResult;
    if (!SessionSearch.IsValid()) {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("No Session Search Relult,Pls using 'find Gmes' first ")));
        return;
    }

	// If the Array is not empty, we can go through it
	if (SessionSearch->SearchResults.Num() <= 0)
	{
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("No Session found!!!,Can NOT do join!")));
        return;
    }
    
    // find sessions,join first session
    GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("FindSessions Num: %d "), SessionSearch->SearchResults.Num()));
    for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
    {
        // To avoid something crazy, we filter sessions from ourself
        if (SessionSearch->SearchResults[i].Session.OwningUserId != Player->GetPreferredUniqueNetId())
        {
            SearchResult = SessionSearch->SearchResults[i];

            // Once we found sounce a Session that is not ours, just join it. Instead of using a for loop, you could
            // use a widget where you click on and have a reference for the GameSession it represents which you can use
            // here
            JoinSessionA(Player->GetPreferredUniqueNetId(), GameSessionName, SearchResult);
            break;
        }
    }
}

void UtrnetDemoGameInstance::DestroySession()
{
    IOnlineSessionPtr Sessions = GetSession();
	
    if (Sessions.IsValid())
    {
        OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
        Sessions->DestroySession(GameSessionName);
    }
}

IOnlineSessionPtr UtrnetDemoGameInstance::GetSession()
{
    UWorld* world = GetWorld();
    check(world != nullptr);
    return Online::GetSessionInterface(world);
}








