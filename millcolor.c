/* GStreamer
 * Copyright 2010 Nick Ludlam <nick@recoil.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-millcolor
 *
 * The millcolor element performs simple colour conversion via lookup tables
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstmillcolor.h"

#include <gst/gst.h>
#include <gst/video/video.h>

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (mill_color_debug);
#define GST_CAT_DEFAULT mill_color_debug

/* elementfactory information */
static const GstElementDetails gst_mill_color_details = GST_ELEMENT_DETAILS ("MillColor filter", "Filter/Effect/Video", "Applies a look from the Mill Colour iPhone app to a video", "Nick Ludlam <nick@recoil.com>");

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS (GST_VIDEO_CAPS_RGBA ";" GST_VIDEO_CAPS_BGRA));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("AYUV")));

GST_BOILERPLATE (GstMillColor, gst_mill_color, GstBaseTransform, GST_TYPE_BASE_TRANSFORM);

static GstCaps *gst_mill_color_transform_caps (GstBaseTransform * btrans, GstPadDirection direction, GstCaps * caps);
static gboolean gst_mill_color_set_caps (GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_mill_color_transform_ip (GstBaseTransform * btrans, GstBuffer * inbuf);

static void gst_mill_color_base_init (gpointer g_class) {
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details (element_class, &gst_mill_color_details);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
}

static void gst_alpha_color_class_init (GstMillColorClass * klass) {
  GstBaseTransformClass *gstbasetransform_class;

  gstbasetransform_class = (GstBaseTransformClass *) klass;

  gstbasetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_mill_color_transform_caps);
  gstbasetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_mill_color_set_caps);
  gstbasetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_mill_color_transform_ip);

  GST_DEBUG_CATEGORY_INIT (mill_color_debug, "millcolor", 0,
      "RGB->YUV conversion via a Mill Colour lookup");
}

static void
gst_alpha_color_init (GstMillColor * mc, GstMillColorClass * g_class)
{
  GstBaseTransform *btrans = NULL;

  btrans = GST_BASE_TRANSFORM (mc);

  btrans->always_in_place = TRUE;
}

static GstCaps *
gst_mill_color_transform_caps (GstBaseTransform * btrans,
GstPadDirection direction, GstCaps * caps)
{
  const GstCaps *tmpl_caps = NULL;
  GstCaps *result = NULL, *local_caps = NULL;
  guint i;

  local_caps = gst_caps_copy (caps);

  for (i = 0; i < gst_caps_get_size (local_caps); i++) {
    GstStructure *structure = gst_caps_get_structure (local_caps, i);

    /* Throw away the structure name and set it to transformed format */
    if (direction == GST_PAD_SINK) {
      gst_structure_set_name (structure, "video/x-raw-yuv");
    } else if (direction == GST_PAD_SRC) {
      gst_structure_set_name (structure, "video/x-raw-rgb");
    }
    /* Remove any specific parameter from the structure */
    gst_structure_remove_field (structure, "format");
    gst_structure_remove_field (structure, "endianness");
    gst_structure_remove_field (structure, "depth");
    gst_structure_remove_field (structure, "bpp");
    gst_structure_remove_field (structure, "red_mask");
    gst_structure_remove_field (structure, "green_mask");
    gst_structure_remove_field (structure, "blue_mask");
    gst_structure_remove_field (structure, "alpha_mask");
  }

  /* Get the appropriate template */
  if (direction == GST_PAD_SINK) {
    tmpl_caps = gst_static_pad_template_get_caps (&src_template);
  } else if (direction == GST_PAD_SRC) {
    tmpl_caps = gst_static_pad_template_get_caps (&sink_template);
  }

  /* Intersect with our template caps */
  result = gst_caps_intersect (local_caps, tmpl_caps);

  gst_caps_unref (local_caps);
  gst_caps_do_simplify (result);

  GST_LOG ("transformed %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT, caps, result);

  return result;
}

static gboolean
gst_mill_color_set_caps (GstBaseTransform * btrans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstMillColor *mc;
  GstStructure *structure;
  gboolean ret;
  const GValue *fps;
  gint red_mask, alpha_mask;
  gint w, h, depth, bpp;

  mc = GST_MILL_COLOR (btrans);
  structure = gst_caps_get_structure (incaps, 0);

  ret = gst_structure_get_int (structure, "width", &w);
  ret &= gst_structure_get_int (structure, "height", &h);
  fps = gst_structure_get_value (structure, "framerate");
  ret &= (fps != NULL && GST_VALUE_HOLDS_FRACTION (fps));
  ret &= gst_structure_get_int (structure, "red_mask", &red_mask);

  /* make sure these are really full RGBA caps */
  ret &= gst_structure_get_int (structure, "alpha_mask", &alpha_mask);
  ret &= gst_structure_get_int (structure, "depth", &depth);
  ret &= gst_structure_get_int (structure, "bpp", &bpp);

  if (!ret || alpha_mask == 0 || red_mask == 0 || depth != 32 || bpp != 32) {
    GST_DEBUG_OBJECT (mc, "incomplete or non-RGBA input caps!");
    return FALSE;
  }

  mc->in_width = w;
  mc->in_height = h;
  mc->in_rgba = TRUE;
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  if (red_mask != 0x000000ff)
#else
  if (red_mask != 0xff000000)
#endif
    mc->in_rgba = FALSE;

  return TRUE;
}

static void
transform_rgb (guint8 * data, gint size)
{
  guint8 r, g, b;
  guint8 y, u, v;

  while (size > 0) {
    r = cross_process_lut[data[0]];
    g = cross_process_lut[data[1] + 256];
    b = cross_process_lut[data[2] + 512];
    
    y = r * 0.299 + g * 0.587 + b * 0.114 + 0;
    u = r * -0.169 + g * -0.332 + b * 0.500 + 128;
    v = r * 0.500 + g * -0.419 + b * -0.0813 + 128;

    data[0] = data[3];
    data[1] = y;
    data[2] = u;
    data[3] = v;

    data += 4;
    size -= 4;
  }
}

static void
transform_bgr (guint8 * data, gint size)
{
  guint8 r, g, b;
  guint8 y, u, v;

  while (size > 0) {
    r = cross_process_lut[data[2]];
    g = cross_process_lut[data[1] + 256];
    b = cross_process_lut[data[0] + 512];
    
    y = r * 0.299 + g * 0.587 + b * 0.114 + 0;
    u = r * -0.169 + g * -0.332 + b * 0.500 + 128;
    v = r * 0.500 + g * -0.419 + b * -0.0813 + 128;

    data[0] = data[3];
    data[1] = y;
    data[2] = u;
    data[3] = v;

    data += 4;
    size -= 4;
  }
}

static GstFlowReturn
gst_mill_color_transform_ip (GstBaseTransform * btrans, GstBuffer * inbuf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstMillColor *mc;

  mc = GST_MILL_COLOR (btrans);

  /* Transform in place */
  if (mc->in_rgba)
    transform_rgb (GST_BUFFER_DATA (inbuf), GST_BUFFER_SIZE (inbuf));
  else
    transform_bgr (GST_BUFFER_DATA (inbuf), GST_BUFFER_SIZE (inbuf));

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "millcolor", GST_RANK_NONE,
      GST_TYPE_MILL_COLOR);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "millcolor",
    "RGB->YUV conversion via a Mill Colour lookup",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)

  static guint8 cross_process_lut[768] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 51, 52, 53, 54, 55, 56, 57, 58, 59, 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 79, 80, 81, 82, 84, 85, 86, 87, 89, 90, 91, 92, 94, 95, 96, 98, 99, 100, 101, 103, 104, 105, 107, 108, 109, 111, 112, 113, 115, 116, 117, 119, 120, 121, 122, 124, 125, 126, 128, 129, 130, 132, 133, 134, 136, 137, 138, 140, 141, 142, 144, 145, 146, 148, 149, 150, 151, 153, 154, 155, 157, 158, 159, 161, 162, 163, 164, 166, 167, 168, 169, 171, 172, 173, 174, 176, 177, 178, 179, 181, 182, 183, 184, 185, 187, 188, 189, 190, 191, 192, 194, 195, 196, 197, 198, 199, 200, 201, 202, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 223, 224, 225, 226, 227, 227, 228, 229, 230, 230, 231, 232, 232, 233, 234, 234, 235, 236, 236, 237, 237, 238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 253, 253, 253, 254, 254, 255, 255, 255, 
    0, 0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 18, 18, 19, 19, 20, 21, 21, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28, 29, 30, 31, 31, 32, 33, 34, 35, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 57, 58, 59, 60, 61, 63, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 88, 89, 90, 92, 93, 95, 96, 98, 99, 100, 102, 103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 144, 146, 147, 149, 150, 152, 153, 155, 156, 158, 159, 161, 163, 164, 166, 167, 169, 170, 172, 173, 175, 176, 178, 179, 181, 182, 183, 185, 186, 188, 189, 191, 192, 193, 195, 196, 198, 199, 201, 202, 203, 205, 206, 208, 209, 211, 212, 214, 215, 216, 218, 219, 221, 222, 224, 225, 227, 228, 229, 231, 232, 234, 235, 237, 238, 240, 241, 243, 244, 246, 247, 248, 250, 251, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    26, 27, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 45, 45, 46, 47, 48, 48, 49, 50, 51, 51, 52, 53, 53, 54, 55, 56, 56, 57, 58, 59, 59, 60, 61, 62, 62, 63, 64, 65, 65, 66, 67, 68, 68, 69, 70, 71, 71, 72, 73, 74, 74, 75, 76, 77, 77, 78, 79, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 88, 89, 90, 91, 91, 92, 93, 94, 94, 95, 96, 97, 97, 98, 99, 100, 100, 101, 102, 103, 103, 104, 105, 106, 106, 107, 108, 109, 109, 110, 111, 112, 112, 113, 114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 121, 122, 123, 124, 124, 125, 126, 126, 127, 128, 129, 129, 130, 131, 132, 132, 133, 134, 135, 135, 136, 137, 138, 138, 139, 140, 141, 141, 142, 143, 144, 144, 145, 146, 147, 147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 156, 156, 157, 158, 159, 159, 160, 161, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 182, 182, 183, 184, 185, 185, 186, 187, 188, 188, 189, 190, 191, 191, 192, 193, 194, 194, 195, 196, 196, 197, 198, 199, 199, 200, 201, 202, 202, 203, 204, 205, 205, 206, 207, 208, 208, 209, 210, 211, 211, 212, 213, 214, 214, 215, 216 
    };
