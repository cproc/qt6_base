/*
 * \brief  Genode QPA plugin
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _QGENODEINTEGRATIONPLUGIN_H_
#define _QGENODEINTEGRATIONPLUGIN_H_

/* Genode includes */
#include <base/env.h>

/* Qt includes */
#include <qpa/qplatformintegrationplugin.h>
#include "qgenodeintegration.h"

QT_BEGIN_NAMESPACE

class QGenodeIntegrationPlugin : public QPlatformIntegrationPlugin
{
	Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "genode.json")

private:
	static Genode::Env *_env;

public:
    QStringList keys() const;
    QPlatformIntegration *create(const QString&, const QStringList&) override;

	static void env(Genode::Env &env) { _env = &env; }
};

QT_END_NAMESPACE

#endif /* _QGENODEINTEGRATIONPLUGIN_H_ */
