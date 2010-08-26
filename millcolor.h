/* GStreamer millcolor element
 * Copyright (C) 2010 Nick Ludlam <nick@recoil.org>
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

#ifndef _GST_MILL_COLOR_H_
#define _GST_MILL_COLOR_H_

#include <gst/base/gstbasetransform.h>

#define GST_TYPE_MILL_COLOR \
  (gst_mill_color_get_type())
#define GST_MILL_COLOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MILL_COLOR,GstMillColor))
#define GST_MILL_COLOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MILL_COLOR,GstMillColorClass))
#define GST_IS_MILL_COLOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MILL_COLOR))
#define GST_IS_MILL_COLOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MILL_COLOR))

typedef struct _GstMillColor GstMillColor;
typedef struct _GstMillColorClass GstMillColorClass;

struct _GstMillColor
{
  GstBaseTransform element;

  /*< private >*/
  /* caps */
  gint in_width, in_height;
  gboolean in_rgba;
  gint out_width, out_height;
};

struct _GstMillColorClass
{
  GstBaseTransformClass parent_class;
};

GType   gst_mill_color_get_type (void);

#endif /* _GST_MILL_COLOR_H_ */
