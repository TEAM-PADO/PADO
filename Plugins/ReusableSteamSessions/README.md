# Reusable Steam Sessions

`UReusableSteamSessionSubsystem` is a project-independent `UGameInstanceSubsystem` for listen-server sessions. It supports session creation, discovery, joining, destruction, Steam invites, full-session checks, and safe replacement of a pre-existing local session.

## Install in another Unreal project

1. Copy the `ReusableSteamSessions` folder into that project's `Plugins` folder and enable **Reusable Steam Sessions** in the Plugin browser.
2. Add `OnlineSubsystemSteam` and (when using SteamSockets) `SteamSockets` to the target project's `.uproject` plugins list.
3. Add the following Steam configuration, replacing `SteamDevAppId` before release. Keep this configuration in the consuming project because App IDs, net drivers, and shipping policy are game-specific.

```ini
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
bInitServerOnClient=true
SteamDevAppId=480
```

4. In Blueprint, call `Get Game Instance Subsystem` for `ReusableSteamSessionSubsystem`, bind its events once, then call the desired operation.

Set a unique `ProductId` (for example, `com.studio.mygame`) on both Create and Find. This is required to avoid seeing unrelated games while using Steam's shared test App ID 480. Set `ListenServerMap` if creation should automatically travel to that map with `?listen`; otherwise perform your own server travel when `OnCreateComplete` succeeds.

## Safety behavior

- Connection limits are clamped to 1–100, and joining/inviting a full session is rejected before an OSS request.
- Invalid/expired search results, absent local players, unavailable OnlineSubsystems, duplicate operations, and failed connect-string resolution produce failure events instead of travel.
- Creating or joining while already in a session destroys the old session first, then continues only after its completion callback succeeds.
- Room names are Base64-advertised to avoid Steam OSS non-ASCII metadata corruption; displayed search results are decoded automatically.

The plugin intentionally contains no widgets, loading screen, game-mode, player-controller, map enum, or ProjectKC include. Bind `OnCreateComplete`, `OnJoinComplete`, and `OnFindComplete` from each game's UI/application layer.
