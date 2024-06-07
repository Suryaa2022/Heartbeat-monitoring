
#include <gst/gst.h>
#include <gst/video/video.h>

typedef struct _GstVideoBalance GstVideoBalance;
typedef struct _GstVideoBalanceClass GstVideoBalanceClass;

struct _GstVideoBalance {
  GstBaseTransform element;
  gdouble brightness;
  gdouble contrast;
  gdouble hue;
  gdouble saturation;
};

struct _GstVideoBalanceClass {
  GstBaseTransformClass parent_class;
};

GType gst_video_balance_get_type(void);

static void gst_video_balance_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_video_balance_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

static gboolean gst_video_balance_transform_ip(GstBaseTransform *trans, GstBuffer *buffer);

G_DEFINE_TYPE(GstVideoBalance, gst_video_balance, GST_TYPE_BASE_TRANSFORM);

enum {
  PROP_0,
  PROP_BRIGHTNESS,
  PROP_CONTRAST,
  PROP_HUE,
  PROP_SATURATION
};

static void gst_video_balance_class_init(GstVideoBalanceClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS(klass);

  object_class->set_property = gst_video_balance_set_property;
  object_class->get_property = gst_video_balance_get_property;

  trans_class->transform_ip = gst_video_balance_transform_ip;

  g_object_class_install_property(object_class, PROP_BRIGHTNESS,
                                  g_param_spec_double("brightness", "Brightness",
                                                     "Brightness adjustment",
                                                     -1.0, 1.0, 0.0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(object_class, PROP_CONTRAST,
                                  g_param_spec_double("contrast", "Contrast",
                                                     "Contrast adjustment",
                                                     0.0, 2.0, 1.0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(object_class, PROP_HUE,
                                  g_param_spec_double("hue", "Hue",
                                                     "Hue adjustment",
                                                     -1.0, 1.0, 0.0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(object_class, PROP_SATURATION,
                                  g_param_spec_double("saturation", "Saturation",
                                                     "Saturation adjustment",
                                                     0.0, 2.0, 1.0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_video_balance_init(GstVideoBalance *self) {
  self->brightness = 0.0;
  self->contrast = 1.0;
  self->hue = 0.0;
  self->saturation = 1.0;
}

static void gst_video_balance_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  GstVideoBalance *self = GST_VIDEO_BALANCE(object);

  switch (property_id) {
    case PROP_BRIGHTNESS:
      self->brightness = g_value_get_double(value);
      break;
    case PROP_CONTRAST:
      self->contrast = g_value_get_double(value);
      break;
    case PROP_HUE:
      self->hue = g_value_get_double(value);
      break;
    case PROP_SATURATION:
      self->saturation = g_value_get_double(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void gst_video_balance_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  GstVideoBalance *self = GST_VIDEO_BALANCE(object);

  switch (property_id) {
    case PROP_BRIGHTNESS:
      g_value_set_double(value, self->brightness);
      break;
    case PROP_CONTRAST:
      g_value_set_double(value, self->contrast);
      break;
    case PROP_HUE:
      g_value_set_double(value, self->hue);
      break;
    case PROP_SATURATION:
      g_value_set_double(value, self->saturation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static gboolean gst_video_balance_transform_ip(GstBaseTransform *trans, GstBuffer *buffer) {
  GstVideoBalance *self = GST_VIDEO_BALANCE(trans);
  GstVideoFrame frame;
  guint8 *data;
  gint width, height;
  gint i, j;

  if (!gst_video_frame_map(&frame, buffer, GST_MAP_READWRITE)) {
    return FALSE;
  }

  width = GST_VIDEO_FRAME_WIDTH(&frame);
  height = GST_VIDEO_FRAME_HEIGHT(&frame);
  data = GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);

  // Apply brightness adjustment
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      guint8 *pixel = data + (i * width * 3) + (j * 3);
      pixel[0] += self->brightness * 255; // adjust red component
      pixel[1] += self->brightness * 255; // adjust green component
      pixel[2] += self->brightness * 255; // adjust blue component
    }
  }

  // Apply contrast adjustment
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      guint8 *pixel = data + (i * width * 3) + (j * 3);
      pixel[0] = CLAMP((pixel[0] - 128) * self->contrast + 128, 0, 255); // adjust red component
      pixel[1] = CLAMP((pixel[1] - 128) * self->contrast + 128, 0, 255); // adjust green component
      pixel[2] = CLAMP((pixel[2] - 128) * self->contrast + 128, 0, 255); // adjust blue component
    }
  }

  // Apply hue adjustment
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      guint8 *pixel = data + (i * width * 3) + (j * 3);
      guint8 r = pixel[0];
      guint8 g = pixel[1];
      guint8 b = pixel[2];
      pixel[0] = CLAMP(r + self->hue * (g - b), 0, 255); // adjust red component
      pixel[1] = CLAMP(g + self->hue * (b - r), 0, 255); // adjust green component
      pixel[2] = CLAMP(b + self->hue * (r - g), 0, 255); // adjust blue component
    }
  }

  // Apply saturation adjustment
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      guint8 *pixel = data + (i * width * 3) + (j * 3);
      guint8 r = pixel[0];
      guint8 g = pixel[1];
      guint8 b = pixel[2];
      guint8 avg = (r + g + b) / 3;
      pixel[0] = CLAMP(avg + self->saturation * (r - avg), 0, 255); // adjust red component
      pixel[1] = CLAMP(avg + self->saturation * (g - avg), 0, 255); // adjust green component
      pixel[2] = CLAMP(avg + self->saturation * (b - avg), 0, 255); // adjust blue component
    }
  }

  gst_video_frame_unmap(&frame);
  return TRUE;
}

static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "videobalance", GST_RANK_NONE, GST_TYPE_VIDEO_BALANCE);
}

GST_PLUGIN_DEFINE(videobalance,
    "Video Balance Adjustment Element",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
);
