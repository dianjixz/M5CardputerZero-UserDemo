#include "mac_text_renderer.hpp"

#if defined(__APPLE__)

#include <algorithm>
#include <AppKit/AppKit.h>
#include <cmath>
#include <CoreGraphics/CoreGraphics.h>
#include <filesystem>

namespace {

NSFont *preferred_font(float point_size, bool bold)
{
    NSFont *font = [NSFont fontWithName:@"PingFang SC" size:point_size];
    if (font != nil) {
        return font;
    }
    if (@available(macOS 10.11, *)) {
        return [NSFont systemFontOfSize:point_size
                                 weight:(bold ? NSFontWeightSemibold : NSFontWeightRegular)];
    }
    return bold ? [NSFont boldSystemFontOfSize:point_size] : [NSFont systemFontOfSize:point_size];
}

}  // namespace

bool render_mac_text_png(const std::string &path,
                         const std::string &text,
                         int width,
                         int height,
                         const MacTextRenderStyle &style)
{
    if (path.empty() || text.empty() || width <= 0 || height <= 0) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path output_path(path);
    if (output_path.has_parent_path()) {
        std::filesystem::create_directories(output_path.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    @autoreleasepool {
        NSString *ns_text = [[NSString alloc] initWithBytes:text.data()
                                                    length:text.size()
                                                  encoding:NSUTF8StringEncoding];
        if (ns_text == nil) {
            return false;
        }

        NSMutableParagraphStyle *paragraph = [[NSMutableParagraphStyle alloc] init];
        paragraph.alignment = style.center ? NSTextAlignmentCenter : NSTextAlignmentLeft;
        paragraph.lineBreakMode = style.wrap ? NSLineBreakByWordWrapping : NSLineBreakByTruncatingTail;

        NSColor *color = [NSColor colorWithSRGBRed:style.r / 255.0
                                             green:style.g / 255.0
                                              blue:style.b / 255.0
                                             alpha:style.a / 255.0];

        NSDictionary *attrs = @{
            NSFontAttributeName : preferred_font(style.point_size, style.bold),
            NSForegroundColorAttributeName : color,
            NSParagraphStyleAttributeName : paragraph,
        };

        NSAttributedString *attributed = [[NSAttributedString alloc] initWithString:ns_text attributes:attrs];

        CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        CGContextRef context = CGBitmapContextCreate(nullptr,
                                                     static_cast<size_t>(width),
                                                     static_cast<size_t>(height),
                                                     8,
                                                     static_cast<size_t>(width) * 4,
                                                     color_space,
                                                     kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
        CGColorSpaceRelease(color_space);
        if (context == nullptr) {
            return false;
        }

        CGContextClearRect(context, CGRectMake(0, 0, width, height));

        NSGraphicsContext *graphics = [NSGraphicsContext graphicsContextWithCGContext:context flipped:NO];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:graphics];

        NSUInteger options = NSStringDrawingUsesFontLeading | NSStringDrawingUsesLineFragmentOrigin;
        if (!style.wrap) {
            options |= NSStringDrawingTruncatesLastVisibleLine;
        }

        NSRect draw_rect = NSMakeRect(0, 0, width, height);
        if (!style.wrap) {
            NSRect bounds = [attributed boundingRectWithSize:NSMakeSize(width, CGFLOAT_MAX) options:options];
            const CGFloat text_height = ceil(bounds.size.height);
            const CGFloat origin_y = std::max<CGFloat>(0.0, (height - text_height) / 2.0);
            draw_rect = NSMakeRect(0, origin_y, width, text_height);
        }

        [attributed drawWithRect:draw_rect options:options];
        [NSGraphicsContext restoreGraphicsState];

        CGImageRef image = CGBitmapContextCreateImage(context);
        CGContextRelease(context);
        if (image == nullptr) {
            return false;
        }

        NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithCGImage:image];
        CGImageRelease(image);
        if (bitmap == nil) {
            return false;
        }

        NSData *png = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        if (png == nil) {
            return false;
        }

        NSString *output = [NSString stringWithUTF8String:path.c_str()];
        return output != nil && [png writeToFile:output atomically:YES];
    }
}

#else

bool render_mac_text_png(const std::string &, const std::string &, int, int, const MacTextRenderStyle &)
{
    return false;
}

#endif