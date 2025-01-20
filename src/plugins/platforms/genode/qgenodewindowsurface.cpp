/*
 * \brief  QGenodeWindowSurface
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <blit/blit.h>

/* Qt includes */
#include <QPainter>

#include <private/qguiapplication_p.h>

#include <qpa/qplatformscreen.h>

#include "qgenodeplatformwindow.h"

#include "qgenodewindowsurface.h"

#include <QDebug>

static const bool verbose = false;

QT_BEGIN_NAMESPACE

QGenodeWindowSurface::QGenodeWindowSurface(QWindow *window)
: QPlatformBackingStore(window) { }

QPaintDevice *QGenodeWindowSurface::paintDevice()
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::paintDevice()";

	QGenodePlatformWindow *platform_window = static_cast<QGenodePlatformWindow*>(window()->handle());
	QRect geo = platform_window->geometry();

	if (geo.size() != _image.size()) {
		QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
		_image = QImage(geo.width(), geo.height(), format);
	}

	if (verbose)
		qDebug() << "QGenodeWindowSurface::paintDevice() finished";

	return &_image;
}

void QGenodeWindowSurface::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::flush("
		         << "window =" << window
		         << ", region =" << region
		         << ", offset =" << offset
		         << ")";

	if (offset != QPoint(0, 0))
		Genode::warning("QGenodeWindowSurface::flush(): offset not handled");

	QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
	QGenodePlatformWindow *platform_window = static_cast<QGenodePlatformWindow*>(window->handle());
	QRect geo = platform_window->geometry();

	QImage framebuffer_image(platform_window->framebuffer(),
	                         geo.width(), geo.height(),
	                         format);

	QPainter framebuffer_painter(&framebuffer_image);

	for (QRect rect : region) {

		/*
		 * It happened that after resizing a window, the given flush region was
		 * bigger than the current window size, so clipping is necessary here.
		 */

		rect &= _image.rect();

		framebuffer_painter.drawImage(rect, _image, rect);

		platform_window->refresh(rect.x(),
		                         rect.y(),
		                         rect.width(),
		                         rect.height());
	}
}

QImage QGenodeWindowSurface::toImage() const
{
	return _image;
}

void QGenodeWindowSurface::resize(const QSize &size, const QRegion &)
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::resize:" << this << size;

	/*
	 * It happened in the past that this function was called later
	 * than 'paintDevice()`, so the image size gets adapted there
	 * instead.
	 */
}

QT_END_NAMESPACE
