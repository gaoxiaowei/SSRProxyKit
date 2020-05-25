//
//  PotatsoManager.h
//
//  Created by LEI on 4/4/16.
//  Copyright © 2016 TouchingApp. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Potatso : NSObject

+ (NSURL * _Nonnull)sharedUrl;
+ (NSURL * _Nonnull)sharedDatabaseUrl;
+ (NSURL * _Nonnull)sharedGeneralConfUrl;
+ (NSURL * _Nonnull)sharedSocksConfUrl;
+ (NSURL * _Nonnull)sharedProxyConfUrl;
+ (NSURL * _Nonnull)sharedHttpProxyConfUrl;
+ (NSURL * _Nonnull)sharedLogUrl;
+ (NSURL * _Nonnull)sharedVPNBypassListUrl;
+ (NSURL * _Nonnull)sharedVPNACLFileUrl;
@end
