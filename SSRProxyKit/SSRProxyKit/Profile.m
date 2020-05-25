// Generated by json_to_model

#import "Profile.h"
#include <ssrLocal/ssrLocal.h>

@implementation Profile  {
}

- (id)initWithJSONDictionary:(NSDictionary *)dictionary {

    self = [super init];
    if (![dictionary isKindOfClass:[NSDictionary class]]) {
        return nil;
    }

    if (self) {
        NSString *server = dictionary[serverString];
        _server = [server isKindOfClass:[NSString class]] ? server : @"";

        NSNumber *port = dictionary[serverPortString];
        _serverPort = [port isKindOfClass:[NSNumber class]] ? [port integerValue] : 0;

        NSString *remarks = dictionary[remarksString];
        _remarks = [remarks isKindOfClass:[NSString class]] ? remarks : @"";

        NSString *password = dictionary[passwordString];
        _password = [password isKindOfClass:[NSString class]] ? password : @"";

        NSString *method = dictionary[methodString];
        _method = [method isKindOfClass:[NSString class]] ? method : @"";
        
        NSString *protocol = dictionary[protocolString];
        _protocol = [protocol isKindOfClass:[NSString class]] ? protocol : @"";
        
        NSString *protocolParam = dictionary[protocolParamString];
        _protocolParam = [protocolParam isKindOfClass:[NSString class]] ? protocolParam : @"";
        
        NSString *obfs = dictionary[obfsString];
        _obfs = [obfs isKindOfClass:[NSString class]] ? obfs : @"";
        
        NSString *obfsParam = dictionary[obfsParamString];
        _obfsParam = [obfsParam isKindOfClass:[NSString class]] ? obfsParam : @"";

        NSNumber *listenPort = dictionary[listenPortString];
        _listenPort = [listenPort isKindOfClass:[NSNumber class]] ? [listenPort integerValue] : 0;
        
        NSString *ssrToken = dictionary[ssrTokenString];
        _ssrToken = [protocol isKindOfClass:[NSString class]] ? ssrToken : @"";
        
        
    }
    return self;
}

- (id)initWithJSONData:(NSData *)data {
    self = [super init];
    if (self) {
        NSError *error = nil;
        id result = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:&error];
        if (result) {
            self = [self initWithJSONDictionary:result];
        } else {
            return nil;
        }
    }
    return self;
}

- (NSDictionary *)JSONDictionary {
    NSMutableDictionary *dictionary = [[NSMutableDictionary alloc] init];

    dictionary[serverString] = self.server;
    dictionary[serverPortString] = @(self.serverPort);
    dictionary[remarksString] = self.remarks;
    dictionary[passwordString] = self.password;
    dictionary[methodString] = self.method;
    dictionary[protocolString] = self.protocol;
    dictionary[protocolParamString] = self.protocolParam;
    dictionary[obfsString] = self.obfs;
    dictionary[obfsParamString] = self.obfsParam;
    dictionary[listenPortString] = @(self.listenPort);

    return dictionary;
}

- (NSData *)JSONData {
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:[self JSONDictionary] options:0 error:&error];
    if (error) {
        @throw error;
    }
    return data;
}

- (NSString *) server {
    if (_server == nil) {
        _server = @"";
    }
    return _server;
}

- (NSInteger) serverPort {
    if (_serverPort == 0) {
        _serverPort = 443;
    }
    return _serverPort;
}

- (NSString *) remarks {
    if (_remarks == nil) {
        _remarks = @"";
    }
    return _remarks;
}

- (NSString *) password {
    if (_password == nil) {
        _password = @"";
    }
    return _password;
}

- (NSString *) method {
    if (_method.length == 0) {
        _method = [NSString stringWithUTF8String:ss_cipher_name_of_type(ss_cipher_aes_256_cfb)];
    }
    return _method;
}

- (NSString *) protocol {
    if (_protocol.length == 0) {
        _protocol = [NSString stringWithUTF8String:ssr_protocol_name_of_type(ssr_protocol_origin)];
    }
    return _protocol;
}

- (NSString *) protocolParam {
    if (_protocolParam == nil) {
        _protocolParam = @"";
    }
    return _protocolParam;
}

- (NSString *) obfs {
    if (_obfs.length == 0) {
        _obfs = [NSString stringWithUTF8String:ssr_obfs_name_of_type(ssr_obfs_plain)];
    }
    return _obfs;
}

- (NSString *) obfsParam {
    if (_obfsParam == nil) {
        _obfsParam = @"";
    }
    return _obfsParam;
}

@end