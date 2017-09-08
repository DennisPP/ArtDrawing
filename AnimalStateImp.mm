//
//  AnimalStateImp.m
//  AnimalsApp
//
//  Created by dennis on 20/3/2017.
//  Copyright © 2017 dennis. All rights reserved.
//

#import "AnimalStateImp.h"

#import "AFNetworking.h"
#import "NetworkUtils.h"
#import "ImageUtils.h"

#import "PublicData.h"





#include "SystemUtils.h"
#include "AnimalsState.h"

#include "Graphics/GraphicsHeader.h"
#include "Monster.h"
#include "GameWorldBase.h"

#include "TextureRecord.h"
#include <string>
#include <map>
#include <set>

#include "UserItem.h"

@interface AnimalStateImp()
{
    AnimalsState* animalsState;
    NSOperationQueue* networkQueue;
    
    
    std::map<UserItem, TextureRecord*> texturesMap;
    
    double lastQueryStudentTime;
}
@end

@implementation AnimalStateImp

-(id)initWithState:(AnimalsState*)state
{
    self = [super init];
    if( self )
    {
        lastQueryStudentTime = 0.0f;
        self->animalsState = state;
        networkQueue = [[NSOperationQueue alloc] init];
        [networkQueue setMaxConcurrentOperationCount:1];
    }
    return self;
}


-(void)onEnter
{
    if( ! self->animalsState->settings.useOfflineData )
    {
        NSString* param = [PublicData shareInstance].roomParam;
        if( param )
        {
            param = [self modifyParamWithTheFirstStudentUserIDAndGroupID:param];
            [self queryStudentList:param];
        }
        else
        {
            NSLog(@"WARNING:::::Room Param is nil!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        }
    }
}

-(void)update:(double)dt
{
    if( self->animalsState->settings.useOfflineData )
        return;
    
    if( animalsState->totalPlayedTime - lastQueryStudentTime>= 8.0f )
    {
        lastQueryStudentTime = animalsState->totalPlayedTime;
        [self queryStudentList];
    }
}


-(void)queryStudentList
{
    if( self->animalsState->settings.useOfflineData )
        return;
    NSString* param = [PublicData shareInstance].roomParam;
    if( param )
    {
        param = [self modifyParamWithTheFirstStudentUserIDAndGroupID:param];
        [self queryStudentList:param];
    }
    else
    {
        NSLog(@"WARNING:::::Room Param is nil!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
}

-(CGSize)getFitSize:(CGSize)originalSize maxSize:(CGSize)maxSize
{
    if( originalSize.width==0.0f || originalSize.height==0.0f)
        return originalSize;
    float w = originalSize.width;
    float h = originalSize.height;
    if( w >= h )
    {
        if( w > maxSize.width )
        {
            float s = maxSize.width / w;
            float scaledWidth = maxSize.width;
            float scaledHeight = h*s;
            return CGSizeMake(scaledWidth, scaledHeight);
        }
    }
    else
    {
        if( h > maxSize.height )
        {
            float s = maxSize.height / h;
            float scaledHeight = maxSize.height;
            float scaledWidth = w*s;
            return CGSizeMake(scaledWidth, scaledHeight);
        }
    }
    return originalSize;
}



#if TARGET_OS_OSX
- (NSImage *)flipImage:(NSImage *)image
{
    NSImage *tmpImage;
    NSAffineTransform *transform = [NSAffineTransform transform];
    NSSize dimensions = [image size];
    NSAffineTransformStruct flip = {1.0, 0.0, 0.0, -1.0, 0.0,
        dimensions.height};
    tmpImage = [[NSImage alloc] initWithSize:dimensions];
    [tmpImage lockFocus];
    [transform setTransformStruct:flip];
    [transform concat];
    [image drawAtPoint:NSMakePoint(0,0)
                   fromRect:NSMakeRect(0,0, dimensions.width, dimensions.height)
                  operation:NSCompositeCopy fraction:1.0];
    [tmpImage unlockFocus];
    
    return tmpImage;
}
#endif


-(void)makeGLTexture:(Image*)image path:(NSString*)path user:(NSInteger)user animalType:(int)animalType item:(NSDictionary*)dict
{
    if( ! image )
        return;
    
    NSLog(@"%s", __FUNCTION__);
    
    

    
    UserItem item;
    item.drawType = animalType;
    item.userId = std::to_string(user);
    std::string p = std::string([path UTF8String]);
    std::map<UserItem, TextureRecord*>::iterator it = texturesMap.find(item);
    
    TextureRecord* texRecord = NULL;
    GLuint texture = 0;
    if( it==texturesMap.end())
    {
        GLuint newTexture = 0;
        glGenTextures(1,&newTexture);
        texture = newTexture;
        TextureRecord* record = new TextureRecord();
        record->texture = newTexture;
        record->path = p;
        record->user = std::to_string(user);
        record->textureState = TextureRecord::TextureStateUnkwnow;
        texturesMap[item] = record;
        texRecord = record;
    }
    else
    {
        texture = it->second->texture;
        texRecord = it->second;
    }
    
    
    #if 0
    it = texturesMap.begin();
    for(;it!=texturesMap.end();++it)
    {
        printf("TEXTURE MAP:%s\n", it->first.c_str());
    }
    #endif
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        
        
        
        if( texRecord)
            texRecord->textureState = TextureRecord::TextureStateIsLoading;
        
        
        #if TARGET_OS_IPHONE
        
        // 1.
        CGImageRef imageRef = [image CGImage];
        long width = CGImageGetWidth(imageRef);
        long height = CGImageGetHeight(imageRef);
        
        
        CGColorSpaceRef space = CGImageGetColorSpace (imageRef);
        NSLog(@"Space:%@ Size:[%ld %ld]", space, width, height);
        
        // 2.
        __block GLubyte* textureData = new GLubyte[(width * height * 4)]; // if 4 components per pixel (RGBA)
        
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        NSUInteger bytesPerPixel = 4;
        NSUInteger bytesPerRow = bytesPerPixel * width;
        NSUInteger bitsPerComponent = 8;
        CGContextRef context = CGBitmapContextCreate(textureData, width, height,
                                                     bitsPerComponent, bytesPerRow, colorSpace,                                              kCGImageAlphaNoneSkipLast | kCGImageByteOrder32Big);
        
        
        CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0,height);
        CGContextConcatCTM(context, flipVertical);
        
        CGContextSetBlendMode(context, kCGBlendModeCopy);
        
        
        CGColorSpaceRelease(colorSpace);
        
        //float alpha = 1.0f;
        //CGContextSetAlpha(context, alpha);
        
        
        CGContextSetRGBFillColor(context, 1, 1, 1, 1);
        CGContextFillRect(context, CGRectMake(0, 0, width, height));
        //CGContextClearRect( context, CGRectMake( 0, 0, width, height ) );
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), imageRef);
        CGContextRelease(context);
        
        
        assert( glGetError() == GL_NO_ERROR);
        
        #else
        
        NSImage* flipedImage = [self flipImage:image];
        
        NSBitmapImageRep* bitmap = [NSBitmapImageRep alloc];
        int samplesPerPixel = 0;
        NSSize imgSize = [flipedImage size];
        [flipedImage lockFocus];
        [bitmap initWithFocusedViewRect:
         NSMakeRect(0.0, 0.0, imgSize.width, imgSize.height)];
        [flipedImage unlockFocus];
        
        long width = flipedImage.size.width;
        long height = flipedImage.size.height;
        NSArray * imageReps = flipedImage.representations;
        for (NSImageRep * imageRep in imageReps)
        {
            if ([imageRep pixelsWide] > width)
                width = [imageRep pixelsWide];
            if ([imageRep pixelsHigh] > height)
                height = [imageRep pixelsHigh];
        }
        
        samplesPerPixel = [bitmap samplesPerPixel];
        __block GLubyte* textureData = NULL;
        // Nonplanar, RGB 24 bit bitmap, or RGBA 32 bit bitmap.
        if(![bitmap isPlanar] &&
           (samplesPerPixel == 3 || samplesPerPixel == 4))
        {
            textureData = [bitmap bitmapData];
        }
        #endif
        dispatch_async(dispatch_get_main_queue(), ^{
            
            // Set proper unpacking row length for bitmap.
            
            
            #if TARGET_OS_OSX
            glPixelStorei(GL_UNPACK_ROW_LENGTH, [bitmap pixelsWide]);
            #endif
            
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glBindTexture(GL_TEXTURE_2D, texture);
            
            printf("Updating Texture:%d:[%d %d]\n", texture, (int)width, (int)height);
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
            
            assert( glGetError() == GL_NO_ERROR);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            #if TARGET_OS_IPHONE
            delete [] textureData;
            textureData = NULL;
            #else
            textureData = NULL;
            // TODO:Free the bitmap
            #endif
            if( texRecord)
                texRecord->textureState= TextureRecord::TextureStateIsReady;
            
            
            if( animalsState)
            {
                std::string userId = std::to_string(user);
                printf("Update texture:%s\n", userId.c_str());
                NSString* date = [dict objectForKey:@"createdate"];
                std::string createDate = std::string([date UTF8String]);
                animalsState->gameWorld->UpdateMonstersTexture(userId, animalType, texture, createDate);
            }
            
            //networkStatusMessage = "Texture Downloaded:" + p;
        });
        
    });
}



-(NSString*)resourceIDFromBuildType:(int)buildType
{
    if( SystemUtils::buildType == 0 )
        return @"-10157";
    else if( SystemUtils::buildType == 1 )
        return @"-11630";
    else if( SystemUtils::buildType == 2 )
        return @"-11631";
    else if( SystemUtils::buildType == 3 )
        return @"-11632";
    
    assert(!"ERROR");
    return @"";
}


-(int)fileNameToArtworkIndex:(NSString*)filename
{
    if( ! filename )
        return -1;
    if( [filename hasPrefix:@"image1_2"] )
        return 0;
    if( [filename hasPrefix:@"image2_2"] )
        return 1;
    if( [filename hasPrefix:@"image3_2"] )
        return 2;
    if( [filename hasPrefix:@"image4_2"] )
        return 3;
    return -1;
}


-(void)queryUserList
{
    int buildType = SystemUtils::GetBuildType();
    if( buildType<0 || buildType>=4 )
        return;
    

    
    
    
    
    


    
    //AppDelegate* app = (AppDelegate*)[[UIApplication sharedApplication] delegate];
    NSString* hostName = [PublicData shareInstance].hostName;
    NSString* userId = [PublicData shareInstance].userid;    ;
    NSString* res_id = [self resourceIDFromBuildType:buildType];
    NSString* path = [NSString stringWithFormat:@"%@/apps/meic/api/meic_get_room_parameter.php?user_id=%@&res_id=%@",
        hostName, userId, res_id];
    
    
    NSLog(@"POST TO:%@", path);
    
    NSURL *url = [NSURL URLWithString:path];
    NSString* postString = @"";
    postString = [postString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    NSLog(@"Post:%@", postString);
    NSData *postData = [postString dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
    NSString *postLength = [NSString stringWithFormat:@"%ld", (unsigned long)[postData length] ];
    NSLog(@"Post Length:%@", postLength);
    
    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
    [request setURL:url];
    [request setHTTPMethod:@"POST"];
    // NOTE:setting time out interval too small will make the request failed
    //[request setTimeoutInterval: 20];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    //[request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];
    
    AFHTTPRequestOperation* operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSData* data = (NSData*)responseObject;
         NSLog(@"Data:%@", data);
         NSString *jsonStr = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
         NSLog(@"Post Response:%@:%@", jsonStr, path);
         
         if( jsonStr )
         {
             [self queryStudentList:jsonStr];
             
         }
         
         //NSError* error = nil;
         //id input = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
         //NSLog(@"%@", input);
         
         
         
     }
                                     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"%@", error);
     }];
    [networkQueue addOperation:operation];
}


-(void)queryStudentListWithUserID:(NSString*)userId
{
    NSLog(@"%s", __FUNCTION__);
    //[networkQueue cancelAllOperations];
    
    
    
    
    
#if 1 // New way to get full student list as required by the server programmer
    //NSString* hostName = @"http://jvsj.mers.hk/";
    
    // P.4A
    //NSString* path = @"http://jvsj.mers.hk/apps/meic/api/meic_get_room_parameter.php?user_id=1705235&res_id=-10157";
    // P.3A
    NSString* path = [NSString stringWithFormat:@"%@%@", @"http://jvsj.mers.hk/apps/meic/api/meic_get_room_parameter.php?res_id=-10157&user_id=", userId];
    int buildType = SystemUtils::GetBuildType();
    if( buildType == 0 )
    {
        path = [NSString stringWithFormat:@"%@%@", @"http://jvsj.mers.hk/apps/meic/api/meic_get_room_parameter.php?res_id=-10157&user_id=", userId];
    }
    else if( buildType==1)
    {
        path = [NSString stringWithFormat:@"%@%@", @"http://jvsj.mers.hk/apps/meic/api/meic_get_room_parameter.php?res_id=-11630&user_id=", userId];
    }
    
    
    
    
    http://jvsj.mers.hk/apps/meic/api/meic_get_room_parameter.php?user_id=1782685&res_id=-10157
    NSURL *url = [NSURL URLWithString:path];
    NSLog(@"Path:%@", path);
    NSString* postString = @"";
#endif
    
    //postString = @"";
    postString = [postString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    NSLog(@"Post:%@", postString);
    NSData *postData = [postString dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
    NSString *postLength = [NSString stringWithFormat:@"%ld", (unsigned long)[postData length] ];
    NSLog(@"Post Length:%@", postLength);
    
    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
    [request setURL:url];
    [request setHTTPMethod:@"POST"];
    // NOTE:setting time out interval too small will make the request failed
    //[request setTimeoutInterval: 20];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    //[request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];
    
    AFHTTPRequestOperation* operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSData* data = (NSData*)responseObject;
         NSLog(@"Data:%@", data);
         NSString *jsonStr = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
         NSLog(@"JSON:%@", jsonStr);
         
         if( jsonStr )
         {
             [self queryStudentList:jsonStr];
         }
         
         //NSError* error = nil;
         //id input = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
         //NSLog(@"%@", input);
         
         
         
         
    }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"%@", error);
     }];
    [networkQueue addOperation:operation];
}








-(NSDictionary*)getLatestValidItem:(NSArray*)items user:(NSInteger)user
{
    int count = (int)[items count];
    NSMutableArray* filteredArray = [[NSMutableArray alloc] init];
    for(int j=0;j<count;++j)
    {
        NSObject* item =  [items objectAtIndex:j];
        NSDictionary* dict = (NSDictionary*)item;
        if( ! dict )
            continue;
        
        NSInteger from_user_id = [[dict objectForKey:@"from_user_id"] integerValue];
        if( from_user_id != user )
            continue;
        
        id data = [dict objectForKey:@"data"];
        NSLog(@"Type for data:%@", NSStringFromClass([data class]));
        if( [data isKindOfClass:[NSNull class]] || ![data isKindOfClass:[NSString class]])
        {
            continue;
        }
        id path = [dict objectForKey:@"path"];
        if( !path || ![path isKindOfClass:[NSString class]])
        {
            NSLog(@"Path is NULL!!!");
            continue;
        }
        [filteredArray addObject:dict];
    }

    
    if( 0==[filteredArray count])
        return nil;
    
    NSArray *sortedArray;
    // TODO:server will return negative timestamp!!!!!
    // 
    sortedArray = [filteredArray sortedArrayUsingComparator:^NSComparisonResult(id a, id b)
    {
        id createdate0 = [a objectForKey:@"createdate"];
        id createdate1 = [b objectForKey:@"createdate"];
        NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
        [dateFormat setDateFormat:@"yyyy-MM-dd' 'HH:mm:ss"];
        NSDate *date0 = [dateFormat dateFromString:createdate0];
        NSDate *date1 = [dateFormat dateFromString:createdate1];
        NSComparisonResult result = [date0 compare:date1];
        return result;
    }];
    return [sortedArray firstObject];
}


-(BOOL)isValidItem:(id)item
{
    if( ! [item isKindOfClass:[NSDictionary class]])
        return NO;
    NSDictionary* itemDict = (NSDictionary*)item;
    if( ! itemDict )
       return NO;
    #if 0
    id data = [itemDict objectForKey:@"data"];
    NSLog(@"Type for data:%@", NSStringFromClass([data class]));
    if( [data isKindOfClass:[NSNull class]] || ![data isKindOfClass:[NSString class]])
        return NO;
    #endif
    
    id path = [itemDict objectForKey:@"path"];
    if( !path || ![path isKindOfClass:[NSString class]])
        return NO;
    
    id filename = [itemDict objectForKey:@"filename"];
    if( !filename || ![filename isKindOfClass:[NSString class]])
        return NO;
    
    if( [self fileNameToArtworkIndex:filename]<0)
        return NO;   
    
    
    id dateStr = [itemDict objectForKey:@"createdate"];
    if( !dateStr || ![dateStr isKindOfClass:[NSString class]])
       return NO;
    
    NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
    [dateFormat setDateFormat:@"yyyy-MM-dd' 'HH:mm:ss"];
    NSDate *date = [dateFormat dateFromString:dateStr];
    if( !date )
        return NO;
    
    return YES;
}


-(BOOL)needUpdate:(id)item monster:(GameObject*)monster
{
    assert([self isValidItem:item]);
    
    NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
    //[dateFormat setDateFormat:@"yyyy-MM-dd'T'HH:mm:ss"];
    [dateFormat setDateFormat:@"yyyy-MM-dd' 'HH:mm:ss"];
    
    NSString* createdate = [item objectForKey:@"createdate"];
    NSDate *date0 = [dateFormat dateFromString:[NSString stringWithFormat:@"%s", monster->createdate.c_str()]];
    NSDate *date1 = [dateFormat dateFromString:createdate];
    NSComparisonResult result = [date0 compare:date1];
    if( result == NSOrderedAscending )
    {
        printf("Need Update!\n");
        return YES;
    }
    return NO;
}


-(int)artworkTypeByString:(NSString*)string
{
    int animalType = 0;
    NSString *animalTypeString = [string stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    
    if( [animalTypeString isEqualToString:@"Artwork01"])
        animalType = 0;
    else if( [animalTypeString isEqualToString:@"Artwork02"])
        animalType = 1;
    else if( [animalTypeString isEqualToString:@"Artwork03"])
        animalType = 2;
    else if( [animalTypeString isEqualToString:@"Artwork04"])
        animalType = 3;
    else
    {
        NSLog(@"WARNING:::::Artwork Type NOT Recognized!!!!");
    }
    return animalType;
}



-(NSString*)modifyParamWithTheFirstStudentUserIDAndGroupID:(NSString*)para
{
    
    NSLog(@"%s:%@", __FUNCTION__, para);
    NSString* userId = [PublicData shareInstance].userid;
    NSString* groupId = [PublicData shareInstance].groupID;
    
    NSData *data = [para dataUsingEncoding:NSUTF8StringEncoding];
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    NSString* newParam = nil;
    if( [json isKindOfClass:[NSDictionary class]])
    {
        NSMutableDictionary* dict = [[NSMutableDictionary alloc] initWithDictionary:json];
        [dict setValue:userId forKey:@"user_id"];
        [dict setValue:groupId forKey:@"ug_id"];
        
        NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dict
                                                           options:NSJSONWritingPrettyPrinted
                                                             error:nil];
        
        newParam = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        
    }
    return newParam;
}


-(void)queryStudentList:(NSString*)param;
{
    NSLog(@"%s", __FUNCTION__);
    
    assert( param );
    
    
    NSString* hostName = [PublicData shareInstance].hostName;
    NSString* path = [NSString stringWithFormat:@"%@%@", hostName,@"/apps/meic/api/meic_get_function_data.php"];
    
    
    NSURL *url = [NSURL URLWithString:path];
    NSLog(@"Path:%@", path);
    NSString* postString = [NSString stringWithFormat:@"para=%@%@", param, @"&function=image%_2&to_user=student&from_user=student"];
    
    //postString = @"";
    postString = [postString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    NSLog(@"Post:%@", postString);
    NSData *postData = [postString dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
    NSString *postLength = [NSString stringWithFormat:@"%ld", (unsigned long)[postData length] ];
    NSLog(@"Post Length:%@", postLength);
    
    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
    [request setURL:url];
    [request setHTTPMethod:@"POST"];
    // NOTE:setting time out interval too small will make the request failed
    //[request setTimeoutInterval: 20];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    //[request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];
    
    AFHTTPRequestOperation* operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSData* data = (NSData*)responseObject;
         NSLog(@"Data:%@", data);
         NSError* error = nil;
         id input = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
         NSLog(@"Post Response:%@:%@", input, path);
         if( ![input isKindOfClass:[NSArray class]])
         {
             NSLog(@"WARNING:Expect Server return Array of object but get %@!!!!", NSStringFromClass([input class]));
         }
         else
         {
             NSArray* resultArray = (NSArray*)input;
             if( resultArray )
             {
                 NSLog(@"%@", [resultArray description]);
                 long count = [resultArray count];
                 NSLog(@"Item Count:%ld", count);
                 if( count == 0)
                     NSLog(@"WARNING:Empty Array from Server!!");
                 for(int j=0;j<count;++j)
                 {
                     NSObject* item =  [resultArray objectAtIndex:j];
                     NSLog(@"value:%@", item);
                     if( ![self isValidItem:item])
                     {
                         NSLog(@"Invalid Item:%s:%@", __FUNCTION__, [item description]);
                     }
                     else
                     {
                            NSDictionary* dict = (NSDictionary*)item;
                        
                            //id data = [dict objectForKey:@"data"];
                            id path = [dict objectForKey:@"path"];
                            id filename = [dict objectForKey:@"filename"];
                            //int animalType = [self artworkTypeByString:data];
                         
                            int animalType = [self fileNameToArtworkIndex:filename];
                         
                             NSInteger from_user_id = [[dict objectForKey:@"from_user_id"] integerValue];
                             NSLog(@"ITEM:%ld:%@\n", (long)from_user_id, path);
                             
                             NSString* from_user_name = [dict objectForKey:@"from_user_name"];
                             NSString* createdate = [dict objectForKey:@"createdate"];
                            std::string date = createdate ? std::string([createdate UTF8String]) : "";
                             std::string displayName = std::string([from_user_name UTF8String]);
                             bool needDownloadImage = false;
                             GameObject* monster = animalsState->gameWorld->GetMonsterByUserId(std::to_string(from_user_id), animalType);
                             if( ! monster )
                             {
                                 animalsState->gameWorld->CreateMonsterIfNOTExist(std::to_string(from_user_id), displayName, date, animalType);
                                 needDownloadImage = true;
                             }
                             monster = animalsState->gameWorld->GetMonsterByUserId(std::to_string(from_user_id), animalType);
                             // Compare date time to see if need to update texture
                             if( monster)
                             {
                                 if( [self needUpdate:item monster:monster])
                                 {
                                     needDownloadImage = true;
                                     
                                 }
                             }
                             if( [path isKindOfClass:[NSString class]])
                             {
                                 path = [NSString stringWithFormat:@"%@/%@", hostName, path];
                                 printf("Image Path::%s\n", [path UTF8String]);
                             }
                             if( path && needDownloadImage )
                             {
                                 NSOperation* downloadOperation = [NetworkUtils downloadImage:path successBlock:^(Image *image, float imageScale)
                                   {
                                       if( image )
                                       {
                                           // NOTE that in retina display iPad, the max size actually is 512x512
                                           CGSize maxSize = CGSizeMake(256.0f, 256.0f);
                                           float w = image.size.width * imageScale;
                                           float h = image.size.height * imageScale;
                                           if( w>maxSize.width || h>maxSize.height)
                                           {
                                               CGSize size = [self getFitSize:CGSizeMake(w,h) maxSize:maxSize];
                                               dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
                                               Image* scaledImage = [ImageUtils getScaledImage:image scaledToSize:size];
                                                   if( scaledImage)
                                                   {
                                                       dispatch_async(dispatch_get_main_queue(), ^{
                                                           [self makeGLTexture:scaledImage path:path user:from_user_id animalType:animalType item:dict];
                                                       });
                                                   }
                                               });
                                           }
                                           else
                                           {
                                               [self makeGLTexture:image path:path user:from_user_id animalType:animalType item:dict];
                                           }
                                       }
                                       else
                                       {
                                           NSLog(@"%s:WARNING:Image Download is NULL", __FUNCTION__);
                                       }
                                       
                                   } failedBlock:^(NSError *error)
                                   {
                                       NSLog(@"FAILED Image Downloaded:%@", path);
                                   }];
                                 
                                 [networkQueue addOperation:downloadOperation];
                             }
                         }
                 }
             }
         }
     }
                                     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"%@", error);
     }];
    
    
    [networkQueue addOperation:operation];
    
}



@end
