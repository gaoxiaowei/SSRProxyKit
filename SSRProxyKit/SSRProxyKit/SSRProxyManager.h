//
//  SSRProxyManager.h
//  SSRProxyKit
//
//  Created by gaoxiaowei on 2020/5/25.
//  Copyright © 2020 allconnect. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef enum : NSUInteger {
    SSRProxyStatusDisconnected,
    SSRProxyStatusDisconnecting,
    SSRProxyStatusConnecting,
    SSRProxyStatusConnected
} SSRProxyStatus;

NSString*const kProxyStausNotificationName =@"com.ssr.prxoy.status.notification";

typedef void(^HttpProxyCompletion)(int port, NSError *error);
typedef void(^ShadowsocksProxyCompletion)(int port, NSError *_Nullable error);


@interface SSRProxyManager : NSObject

@property (nonatomic, readonly) int socksProxyPort;
@property (nonatomic, readonly) int httpProxyPort;
@property (nonatomic, assign,readonly) SSRProxyStatus     status;

+ (SSRProxyManager *)sharedManager;

- (void)startWithCompletion:(void (^)(NSError *error))completion;
- (void)stopWithCompletion:(void (^)(NSError *error))completion;
- (void)updateSSRToken:(NSString*)ssrToken;

@end

NS_ASSUME_NONNULL_END
