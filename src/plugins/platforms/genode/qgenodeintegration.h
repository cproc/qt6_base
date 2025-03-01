/*
 * \brief  QGenodeIntegration
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


#ifndef _QGENODEINTEGRATION_H_
#define _QGENODEINTEGRATION_H_

#include <EGL/egl.h>

#include <QOpenGLContext>

#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>

#include "qgenodescreen.h"
#include "qgenodesignalproxythread.h"

QT_BEGIN_NAMESPACE

class QGenodeIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
	private:

		Genode::Env                           &_env;
		mutable QGenodeSignalProxyThread       _signal_proxy;
		QGenodeScreen                         *_genode_screen;
		QScopedPointer<QPlatformInputContext>  m_inputContext;
		EGLDisplay                             m_eglDisplay;

	public:

		QGenodeIntegration(Genode::Env &env);

		void initialize() Q_DECL_OVERRIDE;
		bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;

		QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
		QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;

		QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;

		QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;

#ifndef QT_NO_CLIPBOARD
		QPlatformClipboard *clipboard() const Q_DECL_OVERRIDE;
#endif
		QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;

		QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;

		QPlatformInputContext *inputContext() const Q_DECL_OVERRIDE;

		QPlatformNativeInterface *nativeInterface() const override;

		// QPlatformNativeInterface
	    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
		void *nativeResourceForIntegration(const QByteArray &resource) override;
};

QT_END_NAMESPACE

#endif /* _QGENODEINTEGRATION_H_ */
