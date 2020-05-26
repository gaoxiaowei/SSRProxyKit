//
//  SSRProxyManager.m
//  SSRProxyKit
//
//  Created by gaoxiaowei on 2020/5/25.
//  Copyright © 2020 allconnect. All rights reserved.
//

#import "SSRProxyManager.h"
#import <ssrLocal/ssrLocal.h>
#import <ShadowPath/ShadowPath.h>
#import "Profile.h"
#import "Potatso.h"
#import "JSONUtils.h"
#import "TunnelError.h"

#if DEBUG
#define WAIT_TIME      20000
#else
#define WAIT_TIME      20
#endif


@interface SSRProxyManager()
{
    int _shadowsocksProxyPort;
    ShadowsocksProxyCompletion _shadowsocksCompletion;
    BOOL _httpProxyRunning;
    HttpProxyCompletion _httpCompletion;
    void (^_pendingStartCompletion)(NSError *);
    
}
@property (nonatomic, strong) dispatch_queue_t   taskQueue;

- (void)onHttpProxyCallback: (int)fd;
- (void)onShadowsocksCallback:(int)fd;

@end

struct ssr_local_state *g_state = NULL;

int sock_port (int fd) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(fd, (struct sockaddr *)&sin, &len) < 0) {
//        NSLog(@"getsock_port(%d) error: %s", fd, strerror (errno));
        return 0;
    } else {
        return ntohs(sin.sin_port);
    }
}

void shadowsocks_handler(int fd, void *udata) {
    SSRProxyManager *provider = (__bridge SSRProxyManager *)udata;
    [provider onShadowsocksCallback:fd];
}

void http_proxy_handler(int fd, void *udata) {
    SSRProxyManager *provider = (__bridge SSRProxyManager *)udata;
    [provider onHttpProxyCallback:fd];
    
}

void feedback_state(struct ssr_local_state *state, void *p) {
    g_state = state;
    shadowsocks_handler(ssr_Local_listen_socket_fd(state), p);
}

struct server_config * build_config_object(Profile *profile, unsigned short listenPort) {
    const char *protocol = profile.protocol.UTF8String;
    if (protocol && strcmp(protocol, "verify_sha1") == 0) {
        // LOGI("The verify_sha1 protocol is deprecate! Fallback to origin protocol.");
        protocol = NULL;
    }
    
    struct server_config *config = config_create();

    config->udp = true;
    config->listen_port = listenPort;
    string_safe_assign(&config->method, profile.method.UTF8String);
    string_safe_assign(&config->remote_host, profile.server.UTF8String);
    config->remote_port = (unsigned short) profile.serverPort;
    string_safe_assign(&config->password, profile.password.UTF8String);
    string_safe_assign(&config->protocol, protocol);
    string_safe_assign(&config->protocol_param, profile.protocolParam.UTF8String);
    string_safe_assign(&config->obfs, profile.obfs.UTF8String);
    string_safe_assign(&config->obfs_param, profile.obfsParam.UTF8String);

    return config;
}

void ssr_main_loop(Profile *profile, unsigned short listenPort, void *context) {
    struct server_config *config = NULL;
    do {
        config = build_config_object(profile, listenPort);
        if (config == NULL) {
            break;
        }

        if (config->method == NULL || config->password==NULL || config->remote_host==NULL) {
            break;
        }

        ssr_local_main_loop(config, &feedback_state, context);
        g_state = NULL;
    } while(0);

    config_release(config);
}

void ssr_stop(void) {
    ssr_token_safe_destroy();
}

void ssr_update_token(const char* ssr_token){
    ssr_local_update_token(ssr_token);
}



@implementation SSRProxyManager

+ (SSRProxyManager *)sharedManager {
    static dispatch_once_t onceToken;
    static SSRProxyManager *manager;
    dispatch_once(&onceToken, ^{
        manager = [SSRProxyManager new];
    });
    return manager;
}

-(id)init{
    self =[super init];
    if(self){
       self.taskQueue = dispatch_queue_create("com.ssr.proxy.queue", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (void)startWithCompletion:(void (^)(NSError *error))completion {
    NSLog(@"SSRProxyManager->startWithCompletion");
    dispatch_async(self.taskQueue, ^{
        dispatch_group_t group = dispatch_group_create();
        dispatch_group_enter(group);
        __block NSError*error_ =nil;
        [self startShadowsocks:^(int port, NSError * _Nullable error1) {
             error_=error1;
             dispatch_group_leave(group);
        }];

        //等待上面的任务完成
        dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

        dispatch_group_enter(group);
        [self startHttpProxy:^(int port, NSError * _Nonnull error2) {
            error_=error2;
            dispatch_group_leave(group);
        }];

        dispatch_group_notify(group, dispatch_get_main_queue(), ^{
            completion(error_);
        });
    });

}

-(void)stopWithCompletion:(void (^)(NSError *error))completion{
    NSLog(@"SSRProxyManager->stopWithCompletion");
    dispatch_async(self.taskQueue, ^{
        [self stopHttpProxy];
        [self stopShadowsocks];
        dispatch_async(dispatch_get_main_queue(), ^{
             completion(nil);
        });
       
    });
}

-(void)updateSSRToken:(NSString*)ssrToken{
    NSLog(@"SSRProxyManager->updateSSRToken:%@",ssrToken);
    dispatch_async(self.taskQueue, ^{
         if(ssrToken!=nil){
            ssr_update_token([ssrToken UTF8String]);
         }
     });
}

# pragma mark - Shadowsocks

- (void)startShadowsocks: (ShadowsocksProxyCompletion)completion {
    _shadowsocksCompletion = [completion copy];
    [NSThread detachNewThreadSelector:@selector(_startShadowsocks) toTarget:self withObject:nil];
}

- (void)_startShadowsocks {
    NSString *confContent = [NSString stringWithContentsOfURL:[Potatso sharedProxyConfUrl] encoding:NSUTF8StringEncoding error:nil];
    NSDictionary *json = [confContent jsonDictionary];
    Profile *profile = [[Profile alloc] initWithJSONDictionary:json];
    profile.listenPort = 0;

    [[NSFileManager defaultManager] removeItemAtURL:[Potatso sharedProxyConfUrl] error:nil];

    if (profile.server.length && profile.serverPort && profile.password.length) {
        ssr_main_loop(profile, profile.listenPort, (__bridge void *)(self));
    }else {
        if (_shadowsocksCompletion) {
            _shadowsocksCompletion(0, nil);
        }
        return;
    }
}

- (void)stopShadowsocks {
    ssr_stop();
}

- (void)onShadowsocksCallback:(int)fd {
    NSError *error;
    if (fd > 0) {
        _shadowsocksProxyPort = sock_port(fd);
    } else {
        error = [NSError errorWithDomain:[[NSBundle mainBundle] bundleIdentifier] code:100 userInfo:@{NSLocalizedDescriptionKey: @"Fail to start http proxy"}];
    }
    if (_shadowsocksCompletion) {
        _shadowsocksCompletion(_shadowsocksProxyPort, error);
    }
}

# pragma mark - Http Proxy

- (void)startHttpProxy:(HttpProxyCompletion)completion {
    _httpCompletion = [completion copy];
    [NSThread detachNewThreadSelector:@selector(_startHttpProxy:) toTarget:self withObject:[Potatso sharedHttpProxyConfUrl]];
}

- (void)_startHttpProxy:(NSURL *)confURL {
    struct forward_spec *proxy = NULL;
    if (_shadowsocksProxyPort > 0) {
        proxy = calloc(1, sizeof(*proxy));
        proxy->type = SOCKS_5;
        proxy->gateway_host = "127.0.0.1";
        proxy->gateway_port = _shadowsocksProxyPort;
    }
    shadowpath_main(strdup([[confURL path] UTF8String]), proxy, http_proxy_handler, (__bridge void *)self);
}

- (void)stopHttpProxy {
    shadowpath_destroy();
    _httpProxyRunning=NO;
}

- (void)onHttpProxyCallback:(int)fd {
    NSError *error;
    if (fd > 0) {
        _httpProxyPort = sock_port(fd);
        _httpProxyRunning = YES;
    }else {
        error = [NSError errorWithDomain:[[NSBundle mainBundle] bundleIdentifier] code:100 userInfo:@{NSLocalizedDescriptionKey: @"Fail to start http proxy"}];
    }
    if (_httpCompletion) {
        _httpCompletion(_httpProxyPort, error);
    }
}



@end
