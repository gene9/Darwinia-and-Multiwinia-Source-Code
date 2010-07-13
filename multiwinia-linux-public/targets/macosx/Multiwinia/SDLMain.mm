/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

extern "C" {
#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import <ASWAboutBox/ASWAboutBox.h>
#import <Sparkle/Sparkle.h>
}
#import "MultiwiniaCocoaBridge.h"

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

#ifdef BETA_EXPIRATION
const volatile int kBetaExpiration = ~1237949505; // 2009-03-25
#endif

extern int WindowManagerSDLIsFullscreen();
extern void WindowManagerSDLPreventFullscreenStartup();
extern void SetWindowed(bool _isWindowed, bool _isPermanent, bool &_isSwitchingToWindowed);

// Called from RunSparkleButton::MouseUp()
void PerformSparkleCheck()
{
	[[NSApp delegate] checkForUpdates:nil];
}

// IOKit callback: pass control back to SDLMain object
static void PowerNotificationReceived(void *_delegate, io_service_t _service,
							   natural_t _messageType, void *_messageArg)
{
	[(id)_delegate receivedPowerNotification:_messageType withArgument:_messageArg];
}

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
	[[NSNotificationCenter defaultCenter] postNotificationName:NSApplicationWillTerminateNotification object:self];
	
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

// Filter out key events in the SDL window, so we don't get to the default handler
// (which just calls NSBeep())
- (void)sendEvent:(NSEvent *)event
{
	BOOL filterEvent = NO;
	
	if ([event type] == NSKeyDown || [event type] == NSKeyUp)
	{
		if ([event modifierFlags] & NSCommandKeyMask)
			filterEvent = NO;
		else if (WindowManagerSDLIsFullscreen())
			filterEvent = YES;
		else if ([[NSApp keyWindow] isKindOfClass:NSClassFromString(@"SDL_QuartzWindow")])
			filterEvent = YES;
	}
	
	if (!filterEvent)
		[super sendEvent:event];
}
- (BOOL)performKeyEquivalent:(NSEvent *)event
{
	return [[NSApp mainMenu] performKeyEquivalent:event];
}
@end

/* The main class of the application, the application's delegate */
@implementation SDLMain
// Initialize the text viewers using RTF files in our application's resources directory
- (id)init
{
	NSString *acknowledgementsFile, *releaseNotesFile;
	
	[super init];
	acknowledgementsFile = [[NSBundle mainBundle] pathForResource:@"Acknowledgements"
												  ofType:@"rtf"];
	_acknowledgements = [[ASWTextViewer alloc] initWithPath:acknowledgementsFile];
	[_acknowledgements setTitle:NSLocalizedString(@"Acknowledgements", @"window title")];
	releaseNotesFile = [[NSBundle mainBundle] pathForResource:@"Release Notes"
											  ofType:@"rtf"];
	_releaseNotes = [[ASWTextViewer alloc] initWithPath:releaseNotesFile];
	[_releaseNotes setTitle:NSLocalizedString(@"Release Notes", @"window title")];
					  	
	return self;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
	if ([menuItem action] == @selector(toggleFullscreen:))
	{
		// HACK: Cocoa menu items can invoked from inside the SDL rendering callback. If
		// the thing we're rendering is a loading screen, that means there's a background
		// loading thread that's making OpenGL load requests, and recreating the OpenGL
		// context right now would be a Bad Thing.
		return !( g_loadingScreen && g_loadingScreen->IsRendering() );
	}
	else if ([menuItem action] == @selector(pauseGame:))
	{
		return WindowManagerSDLGamePaused();
	}
	else
		return YES;
}

- (IBAction) toggleFullscreen:(id)sender
{
	bool didSwitch;
	bool windowed = !WindowManagerSDLIsFullscreen();
	SetWindowed(!windowed, true, didSwitch);
}

// There's a bit of redudancy here, because we want to recover in case the pause
// state changed without our knowledge.
- (IBAction) pauseGame:(id)sender
{
	bool alreadyPaused = WindowManagerSDLGamePaused();
	if (WindowManagerSDLToggleGamePaused())
		[sender setState: alreadyPaused ? NSOffState : NSOnState];
	else
	{
		[sender setState:alreadyPaused ? NSOnState : NSOffState];
		NSBeep();
	}
}

- (IBAction)showAmbrosiaAboutBox:(id)sender
{
	[[ASWAboutBox sharedAboutBox] show];
}

- (IBAction)showAcknowledgements:(id)sender
{
	[_acknowledgements showWindow:sender];
}

- (IBAction)showReleaseNotes:(id)sender
{
	[_releaseNotes showWindow:sender];
}
/* Set the working directory to the Resources/ directory inside the application bundle */
- (void) setupWorkingDirectory
{
	char bundledir[MAXPATHLEN];
	CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());

	if (CFURLGetFileSystemRepresentation(url, true, (UInt8 *)bundledir, MAXPATHLEN)) {
		assert ( chdir (bundledir) == 0 );   /* chdir to the binary app's bundle directory */
		assert ( chdir ("Contents/Resources") == 0 );
	}
	CFRelease(url);
}

/* Called when the internal event loop has just started running */
- (void) applicationWillFinishLaunching: (NSNotification *) note
{
#ifdef BETA_EXPIRATION
	if ([[NSDate date] timeIntervalSince1970] > ~kBetaExpiration)
	{
		NSRunAlertPanel(@"Beta Expired", @"This beta expired on %@. Please download a new version.", @"OK", nil, nil,
						[[NSDateFormatter new] stringFromDate:[NSDate dateWithTimeIntervalSince1970:~kBetaExpiration]]);
		_exit(1);
	}
#endif
	
	_sparkle = [[SUUpdater alloc] init];
	[_sparkle setDelegate:self];
	[_sparkle setModal:YES];
	
	NSNumber *checkAtStartup = [[NSUserDefaults standardUserDefaults] objectForKey:SUCheckAtStartupKey];
	if (!checkAtStartup) {
		checkAtStartup = SUInfoValueForKey(SUCheckAtStartupKey);
	}
	
	if (!checkAtStartup) {
		_firstLaunch = YES; //Nasty workaround so that we don't check for updates twice the first time we run
		[_sparkle performSelector:@selector(showUserChoicePanel)];
		[_sparkle setModal:NO];
	}
	
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:SUAutomaticVersionCheckEnabledKey];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updaterWillDisplay:)
										  name:SUUpdaterWillShowUpdatePanelNotification object:nil];
}

- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	int status;

    /* Set the working directory to the .app's parent directory */
    [self setupWorkingDirectory];
	[[[NSWorkspace sharedWorkspace] notificationCenter]
				   addObserver:self selector:@selector(awakeFromSleep:) name:NSWorkspaceDidWakeNotification object:nil];
	[self registerForSleepNotifications];
	
	/* give the Cocoa code, not SDL, first shot at handling events */
	setenv ("SDL_ENABLEAPPEVENTS", "1", 1);
	/* don't have SDL do mouse-button emulation; we'll handle it ourselves */
	setenv ("SDL_HAS3BUTTONMOUSE", "1", 1);

	[self performSelector:@selector(runSparkle:) withObject:nil afterDelay:0];

    /* Hand off to main application code */
    gCalledAppMainline = TRUE;
    status = SDL_main (gArgc, gArgv);

    /* We're done, thank you for playing */
    exit(status);
}

- (void)runSparkle:(id)sender
{
	[_sparkle setModal:NO];
	
	if (!_firstLaunch && [[[NSUserDefaults standardUserDefaults] objectForKey:SUCheckAtStartupKey] boolValue]) {
		[_sparkle checkForUpdatesInBackground];
	}
}

- (IBAction)checkForUpdates:(id)sender
{
	[_sparkle checkForUpdates:sender];
}

- (void)tryToSwitchToWindowedMode:(NSTimer *)timer
{
	if (!(g_loadingScreen && g_loadingScreen->IsRendering()))
	{
		bool didSwitch;
		SetWindowed(true, false, didSwitch);
		[timer invalidate];
	}
}

- (IBAction)updaterWillDisplay:(id)sender
{
	if (WindowManagerSDLIsFullscreen())
		[NSTimer scheduledTimerWithTimeInterval:0.2 target:self selector:@selector(tryToSwitchToWindowedMode:) userInfo:nil repeats:YES];
	WindowManagerSDLPreventFullscreenStartup();
}

- (IBAction)showUserManual:(id)sender
{
	[[NSWorkspace sharedWorkspace] openFile:[[NSBundle mainBundle] pathForResource:@"UserManual" ofType:@"pdf"]];
}

#pragma mark Sleep Notification

- (void)registerForSleepNotifications
{
	IONotificationPortRef  notifyPort;
	io_object_t notifier;
	
	m_rootPowerDomainPort = IORegisterForSystemPower(self, &notifyPort, PowerNotificationReceived, &notifier);
	if ( m_rootPowerDomainPort != 0 )
	{
		CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notifyPort),
						   kCFRunLoopCommonModes);
	}
}


// Force ourselves into window mode before going to sleep. This keeps Defcon from
// obstructing the authentication dialog if auth-on-wake is enabled.
- (void)receivedPowerNotification:(natural_t)_type withArgument:(void *)_argument
{
	long notificationID = (long)_argument;
	
	switch (_type)
	{
		// Handling this message is required, but we're effectively ignoring it
		case kIOMessageCanSystemSleep:
			IOAllowPowerChange(m_rootPowerDomainPort, notificationID);
			break;
		
		case kIOMessageSystemWillSleep:
			// Since we have to toggle the setting asynchronously, we poll to see when
			// it's been updated and we can go to sleep.
			if (WindowManagerSDLIsFullscreen())
			{
				NSDictionary *userInfo;
				bool didSwitch;
				
				SetWindowed(true, false, didSwitch);
				userInfo = [NSDictionary dictionaryWithObject:[NSNumber numberWithLong:notificationID]
										 forKey:@"notificationID"];
				[NSTimer scheduledTimerWithTimeInterval:0.05 target:self
						 selector:@selector(allowSleepIfReady:)
						 userInfo:userInfo repeats:YES];
			}
			else
				IOAllowPowerChange(m_rootPowerDomainPort, notificationID);
			
			break;
	}
}

- (void)allowSleepIfReady:(NSTimer *)timer
{
	if (!WindowManagerSDLIsFullscreen())
	{
		IOAllowPowerChange(m_rootPowerDomainPort,
						   [[[timer userInfo] objectForKey:@"notificationID"] longValue]);
		[timer invalidate];
	}
}

- (void)awakeFromSleep:(NSNotification *)notification
{
	WindowManagerSDLAwakeFromSleep();
}
@end


#ifdef main
#  undef main
#endif


/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgv = (char **) SDL_malloc(sizeof (char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **) SDL_malloc(sizeof (char *) * (argc+1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }
    NSApplicationMain (argc, (const char **)argv);

    return 0;
}
