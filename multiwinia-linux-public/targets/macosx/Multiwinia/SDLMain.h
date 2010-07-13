/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>
#import <IOKit/IOMessage.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <ASWAppKit/ASWTextViewer.h>
#import <Sparkle/Sparkle.h>

@interface SDLMain : NSObject
{
	ASWTextViewer *_acknowledgements;
	ASWTextViewer *_releaseNotes;
	io_connect_t m_rootPowerDomainPort;
	
	IBOutlet SUUpdater *_sparkle;
	BOOL _firstLaunch;
}
- (void)receivedPowerNotification:(natural_t)_type withArgument:(void *)_argument;
- (void)registerForSleepNotifications;

// These actions are all used by specific menu items
- (IBAction)showAmbrosiaAboutBox:(id)sender;
- (IBAction)showAcknowledgements:(id)sender;
- (IBAction)showReleaseNotes:(id)sender;
- (IBAction)toggleFullscreen:(id)sender;
- (IBAction) pauseGame:(id)sender;
- (IBAction)showUserManual:(id)sender;
@end
