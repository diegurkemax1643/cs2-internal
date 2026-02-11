# Troubleshooting Guide

## Issue: No Overlay After Injection

If you injected the DLL but see nothing:

### 1. Verify DLL is Loaded
- Check Process Hacker - is CS2Internal.dll in the modules list?
- If not, the injection failed

### 2. Check for Crashes
- Open Event Viewer (Windows Logs > Application)
- Look for errors from cs2.exe around injection time
- If you see crashes, the DLL might be causing them

### 3. Test the Hook
- The new build includes a test overlay that should ALWAYS show if the hook works
- You should see "Hook Active!" text in top-left corner
- If you don't see this, the Present hook isn't being called

### 4. Injection Method
- Try different injectors:
  - Extreme Injector (Manual Map)
  - Xenos Injector
  - Process Hacker (native injection)
- Some injectors work better than others

### 5. Timing
- Inject DURING the CS2 loading screen (before main menu)
- Or inject immediately after game window appears
- Don't inject when already in-game

### 6. Verify Dependencies
- Make sure all DLLs are present:
  - d3d11.dll (Windows)
  - dxgi.dll (Windows)
  - The cheat DLL itself

### 7. Check Antivirus
- Some AVs block DLL injection
- Add exception for the DLL folder
- Or temporarily disable AV

### 8. Debug Steps
1. Inject the DLL
2. Wait 2-3 seconds
3. Press INSERT key
4. Check if menu appears (even if overlay doesn't)
5. If menu appears but no overlay, it's a rendering issue
6. If nothing happens, the hook isn't working

## Expected Behavior

After successful injection:
- Small "Hook Active!" window appears in top-left
- Press INSERT to open main menu
- ESP/Aimbot/Triggerbot options available

## If Still Not Working

The hook might be failing silently. The code now:
- Creates temporary swap chain to get Present address
- Hooks Present directly using MinHook
- Should work even if swap chain exists before injection

If it still doesn't work, there might be:
- Anti-cheat interference
- Hook detection
- DLL being unloaded
- Exception during initialization

