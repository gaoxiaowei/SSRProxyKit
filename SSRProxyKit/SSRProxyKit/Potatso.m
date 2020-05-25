//
//  PotatsoManager.m
//
//  Created by LEI on 4/4/16.
//  Copyright © 2016 TouchingApp. All rights reserved.
//

#import "Potatso.h"

@implementation Potatso

+ (NSURL *)sharedUrl {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    return [NSURL fileURLWithPath:documentsDirectory];
}

+ (NSURL *)sharedDatabaseUrl {
    return [[self sharedUrl] URLByAppendingPathComponent:@"potatso.realm"];
}

+ (NSURL * _Nonnull)sharedGeneralConfUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"general.xxx"];
}

+ (NSURL *)sharedSocksConfUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"socks.xxx"];
}

+ (NSURL *)sharedProxyConfUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"proxy.xxx"];
}

+ (NSURL *)sharedHttpProxyConfUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"http.xxx"];
}

+ (NSURL * _Nonnull)sharedLogUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"tunnel.log"];
}

+ (NSURL * _Nonnull)sharedVPNBypassListUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"vpnbypasslist.xxx"];
}

+ (NSURL * _Nonnull)sharedVPNACLFileUrl {
    return [[Potatso sharedUrl] URLByAppendingPathComponent:@"acl.xxx"];
}


@end
