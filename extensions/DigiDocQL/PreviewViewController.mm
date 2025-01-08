/*
 * QEstEidClient
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <digidocpp/Container.h>
#include <digidocpp/DataFile.h>
#include <digidocpp/Signature.h>
#include <digidocpp/Exception.h>
#include <digidocpp/XmlConf.h>

#import <Quartz/Quartz.h>
#import <WebKit/WebKit.h>

using namespace digidoc;

class DigidocConf final: public digidoc::XmlConfCurrent
{
public:
    bool TSLAutoUpdate() const final { return false; }
    bool TSLOnlineDigest() const final { return false; }
    std::string TSLCache() const final
    {
        NSURL *directory = [NSFileManager.defaultManager containerURLForSecurityApplicationGroupIdentifier:@"ET847QJV9F.ee.ria.qdigidoc4.shared"];
        return directory.path.UTF8String;
    }
};

@interface NSString (Digidoc)
+ (NSString*)stdstring:(const std::string&)str;
+ (NSString*)fileSize:(unsigned long)bytes;
+ (void)parseException:(const Exception&)e result:(NSMutableArray *)result;
@end

@implementation NSString (Digidoc)
+ (NSString*)stdstring:(const std::string&)str
{
    return str.empty() ? [NSString string] : [NSString stringWithUTF8String:str.c_str()];
}

+ (NSString*)htmlEntityEncode:(NSString*)str
{
    str = [str stringByReplacingOccurrencesOfString:@"&" withString:@"&amp;"];
    str = [str stringByReplacingOccurrencesOfString:@"\"" withString:@"&quot;"];
    str = [str stringByReplacingOccurrencesOfString:@"'" withString:@"&apos;"];
    str = [str stringByReplacingOccurrencesOfString:@"<" withString:@"&lt;"];
    str = [str stringByReplacingOccurrencesOfString:@">" withString:@"&gt;"];
    return str;
}

+ (NSString*)fileSize:(unsigned long)bytes
{
	if (const auto gb = 1UL << 3; bytes >= gb)
        return [NSString stringWithFormat:@"%1.2f GB", double(bytes) / double(gb)];
	if (const auto mb = 1UL << 2; bytes >= mb)
        return [NSString stringWithFormat:@"%1.2f MB", double(bytes) / double(mb)];
	if (const auto kb = 1UL << 1; bytes >= kb)
        return [NSString stringWithFormat:@"%1.1f KB", double(bytes) / double(kb)];
    return [NSString stringWithFormat:@"%lu bytes", bytes];
}

+ (void)parseException:(const Exception&)e result:(NSMutableArray *)result
{
    [result addObject:[self stdstring:e.msg()]];
    for (const Exception &i : e.causes()) {
        [self parseException:i result:result];
    }
}
@end

@interface PreviewViewController : NSViewController <QLPreviewingController>
@property (weak) IBOutlet WKWebView *webView;
@end

@interface PreviewProvider : QLPreviewProvider <QLPreviewingController>
@end

@implementation PreviewViewController

- (NSString *)nibName {
    return @"PreviewViewController";
}

/*
 * Implement this method and set QLSupportsSearchableItems to YES in the Info.plist of the extension if you support CoreSpotlight.
 *
- (void)preparePreviewOfSearchableItemWithIdentifier:(NSString *)identifier queryString:(NSString *)queryString completionHandler:(void (^)(NSError * _Nullable))handler {
    
    // Perform any setup necessary in order to prepare the view.
    
    // Call the completion handler so Quick Look knows that the preview is fully loaded.
    // Quick Look will display a loading spinner while the completion handler is not called.

    handler(nil);
}
*/

- (void)preparePreviewOfFileAtURL:(NSURL *)url completionHandler:(void (^)(NSError * _Nullable))handler {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSMutableString *h = [NSMutableString string];
        [h appendString:@"<html><head><style>"];
        [h appendString:@"* { font-family: 'Lucida Sans Unicode', 'Lucida Grande', sans-serif };"];
        [h appendString:@"body { font-size: 10pt };"];
        [h appendString:@"h2 { padding-left: 50px; background: url(cid:asic.icns); background-size: 42px 42px; background-repeat:no-repeat; };"];
        [h appendString:@"font, dt { color: #808080 };"];
        [h appendString:@"dt { float: left; clear: left; margin-left: 30px; margin-right: 10px };"];
        [h appendString:@"dl { margin-bottom: 10px };"];
        [h appendString:@"</style></head><body>"];
        [h appendFormat:@"<h2>%@<hr size='1' /></h2>", [NSString htmlEntityEncode:url.lastPathComponent]];
        try
        {
            digidoc::Conf::init(new DigidocConf());
            digidoc::initialize();
            std::unique_ptr<Container> d(Container::openPtr(url.path.UTF8String));

            [h appendString:@"<font>Files</font><ol>"];
            for (const DataFile *doc : d->dataFiles()) {
                [h appendFormat:@"<li>%@</li>", [NSString htmlEntityEncode:[NSString stdstring:doc->fileName()]]];
            }
            [h appendString:@"</ol>"];

            [h appendString:@"<font>Signatures</font>"];
            for (const Signature *s : d->signatures()) {
                [h appendFormat:@"<dl><dt>Signer</dt><dd>%@</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->signedBy()]]];

                NSString *date = [NSString stdstring:s->trustedSigningTime()];
                [date stringByReplacingOccurrencesOfString:@"Z" withString:@"-0000"];
                NSDateFormatter *df = [[NSDateFormatter alloc] init];
                [df setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZ"];
                NSDate *formateddate = [df dateFromString:date];
                [df setTimeZone: [NSTimeZone defaultTimeZone]];
                [df setDateFormat:@"YYYY-MM-dd HH:mm:ss z"];
                [h appendFormat:@"<dt>Time</dt><dd>%@</dd>", [df stringFromDate:formateddate]];

                Signature::Validator v(s);
                NSString *status = @"not valid";
                switch(v.status())
                {
                case Signature::Validator::Valid: status = @"valid"; break;
                case Signature::Validator::Warning: status = @"valid with warnings"; break;
                case Signature::Validator::NonQSCD: status = @"valid with limitations"; break;
                case Signature::Validator::Test: status = @"valid test signature"; break;
                case Signature::Validator::Invalid: status = @"invalid"; break;
                case Signature::Validator::Unknown: status = @"unknown"; break;
                }
                [h appendFormat:@"<dt>Validity</dt><dd>Signature is %@</dd>", status];

                NSMutableArray *roles = [NSMutableArray array];
                for (const std::string &role : s->signerRoles()) {
                    if( !role.empty() ) {
                        [roles addObject:[NSString htmlEntityEncode:[NSString stdstring:role]]];
                    }
                }
                if( [roles count] > 0 ) {
                    [h appendFormat:@"<dt>Role</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[roles componentsJoinedByString:@" / "]]];
                }
                if (!s->countryName().empty()) {
                    [h appendFormat:@"<dt>Country</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->countryName()]]];
                }
                if (!s->city().empty()) {
                    [h appendFormat:@"<dt>City</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->city()]]];
                }
                if (!s->stateOrProvince().empty()) {
                    [h appendFormat:@"<dt>State</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->stateOrProvince()]]];
                }
                if (!s->postalCode().empty()) {
                    [h appendFormat:@"<dt>Postal code</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->postalCode()]]];
                }
                [h appendString:@"</dl>"];
            }
            digidoc::terminate();
        } catch (const Exception &e) {
            NSMutableArray *err = [NSMutableArray array];
            [NSString parseException:e result:err];
            [h appendFormat:@"Failed to load document:<br />%@", [err componentsJoinedByString:@"<br />"]];
        }
        [h appendString:@"</body></html>"];

        dispatch_async(dispatch_get_main_queue(), ^(void) {
            [self.webView loadHTMLString:h baseURL:nil];
            handler(nil);
        });
    });
}

@end

@implementation PreviewProvider
- (void)providePreviewForFileRequest:(QLFilePreviewRequest *)request completionHandler:(void (^)(QLPreviewReply * _Nullable reply, NSError * _Nullable error))handler
{
    QLPreviewReply* reply = [[QLPreviewReply alloc] initWithDataOfContentType:UTTypeHTML contentSize:CGSizeMake(800, 800)
        dataCreationBlock:^NSData * _Nullable(QLPreviewReply * _Nonnull replyToUpdate, NSError *__autoreleasing  _Nullable * _Nullable error) {

        NSMutableString *h = [NSMutableString string];
        [h appendString:@"<html><head><style>"];
        [h appendString:@"* { font-family: 'Lucida Sans Unicode', 'Lucida Grande', sans-serif };"];
        [h appendString:@"body { font-size: 10pt };"];
        [h appendString:@"h2 { padding-left: 50px; background: url(cid:asic.icns); background-size: 42px 42px; background-repeat:no-repeat; };"];
        [h appendString:@"font, dt { color: #808080 };"];
        [h appendString:@"dt { float: left; clear: left; margin-left: 30px; margin-right: 10px };"];
        [h appendString:@"dl { margin-bottom: 10px };"];
        [h appendString:@"</style></head><body>"];
        [h appendFormat:@"<h2>%@<hr size='1' /></h2>", [NSString htmlEntityEncode:request.fileURL.lastPathComponent]];
        try
        {
            digidoc::Conf::init(new DigidocConf());
            digidoc::initialize();
            std::unique_ptr<Container> d(Container::openPtr(request.fileURL.path.UTF8String));

            [h appendString:@"<font>Files</font><ol>"];
            for (const DataFile *doc : d->dataFiles()) {
              [h appendFormat:@"<li>%@</li>", [NSString htmlEntityEncode:[NSString stdstring:doc->fileName()]]];
            }
            [h appendString:@"</ol>"];

            [h appendString:@"<font>Signatures</font>"];
            for (const Signature *s : d->signatures()) {
                [h appendFormat:@"<dl><dt>Signer</dt><dd>%@</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->signedBy()]]];

                NSString *date = [NSString stdstring:s->trustedSigningTime()];
                [date stringByReplacingOccurrencesOfString:@"Z" withString:@"-0000"];
                NSDateFormatter *df = [[NSDateFormatter alloc] init];
                [df setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZ"];
                NSDate *formateddate = [df dateFromString:date];
                [df setTimeZone: [NSTimeZone defaultTimeZone]];
                [df setDateFormat:@"YYYY-MM-dd HH:mm:ss z"];
                [h appendFormat:@"<dt>Time</dt><dd>%@</dd>", [df stringFromDate:formateddate]];

                Signature::Validator v(s);
                NSString *status = @"not valid";
                switch(v.status())
                {
                case Signature::Validator::Valid: status = @"valid"; break;
                case Signature::Validator::Warning: status = @"valid with warnings"; break;
                case Signature::Validator::NonQSCD: status = @"valid with limitations"; break;
                case Signature::Validator::Test: status = @"valid test signature"; break;
                case Signature::Validator::Invalid: status = @"invalid"; break;
                case Signature::Validator::Unknown: status = @"unknown"; break;
                }
                [h appendFormat:@"<dt>Validity</dt><dd>Signature is %@</dd>", status];

                NSMutableArray *roles = [NSMutableArray array];
                for (const std::string &role : s->signerRoles()) {
                    if( !role.empty() ) {
                       [roles addObject:[NSString htmlEntityEncode:[NSString stdstring:role]]];
                    }
                }
                if( [roles count] > 0 ) {
                    [h appendFormat:@"<dt>Role</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[roles componentsJoinedByString:@" / "]]];
                }
                if (!s->countryName().empty()) {
                    [h appendFormat:@"<dt>Country</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->countryName()]]];
                }
                if (!s->city().empty()) {
                    [h appendFormat:@"<dt>City</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->city()]]];
                }
                if (!s->stateOrProvince().empty()) {
                    [h appendFormat:@"<dt>State</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->stateOrProvince()]]];
                }
                if (!s->postalCode().empty()) {
                    [h appendFormat:@"<dt>Postal code</dt><dd>%@&nbsp;</dd>", [NSString htmlEntityEncode:[NSString stdstring:s->postalCode()]]];
                }
                [h appendString:@"</dl>"];
            }
            digidoc::terminate();
        } catch (const Exception &e) {
            NSMutableArray *err = [NSMutableArray array];
            [NSString parseException:e result:err];
            [h appendFormat:@"Failed to load document:<br />%@", [err componentsJoinedByString:@"<br />"]];
        }
        [h appendString:@"</body></html>"];
        return [h dataUsingEncoding:NSUTF8StringEncoding];
    }];
    handler(reply, nil);
}
@end
