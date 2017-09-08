#import <Foundation/Foundation.h>

class AnimalsState;

@interface AnimalStateImp : NSObject

-(id)initWithState:(AnimalsState*)state;

-(void)queryStudentList:(NSString*)param;
//-(void)queryStudentList;
-(void)queryUserList;

-(void)update:(double)dt;
-(void)onEnter;



@end
