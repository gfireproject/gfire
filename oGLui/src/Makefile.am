liboGLuiincludeCOREdir = $(includedir)/oGLui/core
liboGLuiincludeCORE_HEADERS = \
	core/oGLui.h \
	core/oGLuiApplication.h \
	core/oGLuiObject.h \
	core/oGLuiColor.h \
	core/oGLuiColorPalette.h \
	core/oGLuiRect.h \
	core/oGLuiSize.h \
	core/oGLuiPoint.h \
	core/oGLuiPixmap.h \
	core/oGLuiPainter.h \
	core/oGLuiLog.h \
	core/oGLuiLayout.h \
	core/oGLuiLayoutElement.h \
	core/oGLuiLayoutWidgetElement.h \
	core/oGLuiBoxLayout.h \
	core/oGLuiVBoxLayout.h \
	core/oGLuiHBoxLayout.h \
	core/oGLuiFont.h \
	core/oGLuiFontRenderer.h \
	core/oGLuiConfig.h \
	core/oGLuiStyle.h \
	core/oGLuiSolidStyle.h \
	core/oGLuiUtils.h \
	core/oGLuiPointer.h \
	core/oGLuiCRC32.h \
	core/oGLuiResourceManager.h \
	core/oGLuiResource.h \
	core/oGLuiResourcePtr.h

liboGLuiincludeWIDGETSdir = $(includedir)/oGLui/widgets
liboGLuiincludeWIDGETS_HEADERS = \
	widgets/oGLuiWidget.h \
	widgets/oGLuiBaseWidget.h \
	widgets/oGLuiLabel.h \
	widgets/oGLuiButton.h \
	widgets/oGLuiLineEdit.h

liboGLuiincludeEVENTSdir = $(includedir)/oGLui/events
liboGLuiincludeEVENTS_HEADERS = \
	events/oGLuiEvent.h \
	events/oGLuiPaintEvent.h \
	events/oGLuiMouseMoveEvent.h

liboGLui_la_SOURCES = \
	core/oGLuiApplication.cpp \
	core/oGLuiObject.cpp \
	core/oGLuiColor.cpp \
	core/oGLuiColorPalette.cpp \
	core/oGLuiRect.cpp \
	core/oGLuiSize.cpp \
	core/oGLuiPoint.cpp \
	core/oGLuiPixmap.cpp \
	core/oGLuiPainter.cpp \
	core/oGLuiLog.cpp \
	core/oGLuiLayout.cpp \
	core/oGLuiLayoutWidgetElement.cpp \
	core/oGLuiBoxLayout.cpp \
	core/oGLuiFont.cpp \
	core/oGLuiFontRenderer.cpp \
	core/oGLuiConfig.cpp \
	core/oGLuiStyle.cpp \
	core/oGLuiSolidStyle.cpp \
	core/oGLuiUtils.cpp \
	core/oGLuiPointer.cpp \
	core/oGLuiCRC32.cpp \
	core/oGLuiResourceManager.cpp \
	core/oGLuiResource.cpp \
	core/oGLuiResourcePtr.cpp \
	widgets/oGLuiWidget.cpp \
	widgets/oGLuiBaseWidget.cpp \
	widgets/oGLuiLabel.cpp \
	widgets/oGLuiButton.cpp \
	widgets/oGLuiLineEdit.cpp \
	events/oGLuiEvent.cpp \
	events/oGLuiPaintEvent.cpp \
	events/oGLuiMouseMoveEvent.cpp

liboGLui_la_LDFLAGS = -export-dynamic -no-undefined -version-info $(OGLUI_LT_VERSION)
liboGLui_la_LIBADD = $(PANGO_CAIRO_LIBS) $(GL_LIBS) $(GLU_LIBS)

lib_LTLIBRARIES = liboGLui.la

AM_CPPFLAGS = \
	-Wall -O2 $(PANGO_CAIRO_CFLAGS) $(GL_CFLAGS) $(GLU_CFLAGS) -DDATADIR=\"$(datadir)\"