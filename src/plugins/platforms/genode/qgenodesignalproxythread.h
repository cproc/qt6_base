/*
 * \brief  Signal proxy thread to avoid libc execution context nesting issues
 * \author Christian Prochaska
 * \date   2020-08-13
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _QGENODESIGNALPROXYTHREAD_H_
#define _QGENODESIGNALPROXYTHREAD_H_

/* Genode includes */
#include <base/blockade.h>

/* Qt includes */
#include <QThread>

class QGenodeSignalProxyThread : public QThread
{
	Q_OBJECT

	private:

		Genode::Blockade _blockade;

		bool _quit                { false };
		bool _input               { false };
		bool _info_changed        { false };
		bool _screen_info_changed { false };
		bool _clipboard_changed   { false };

	protected:

		void run() override
		{
			for (;;) {

				_blockade.block();

				if (_quit)
					break;

				if (_input) {
					_input = false;
					input_signal();
				}

				if (_info_changed) {
					_info_changed = false;
					info_changed_signal();
				}

				if (_screen_info_changed) {
					_screen_info_changed = false;
					screen_info_changed_signal();
				}

				if (_clipboard_changed) {
					_clipboard_changed = false;
					clipboard_changed_signal();
				}
			}
		}

	public:

		QGenodeSignalProxyThread() { start(); }

		~QGenodeSignalProxyThread()
		{
			_quit = true;
			_blockade.wakeup();
			wait();
		}

		void input()
		{
			_input = true;
			_blockade.wakeup();
		}

		void info_changed()
		{
			_info_changed = true;
			_blockade.wakeup();
		}

		void screen_info_changed()
		{
			_screen_info_changed = true;
			_blockade.wakeup();
		}

		void clipboard_changed()
		{
			_clipboard_changed = true;
			_blockade.wakeup();
		}

	Q_SIGNALS:

		void input_signal();
		void info_changed_signal();
		void screen_info_changed_signal();
		void clipboard_changed_signal();
};

#endif /* _QGENODESIGNALPROXYTHREAD_H_ */
