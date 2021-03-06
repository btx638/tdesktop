/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/linux/file_utilities_linux.h"

#include "platform/linux/linux_gtk_integration.h"
#include "platform/linux/specific_linux.h"

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include "platform/linux/linux_xdp_file_dialog.h"
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#include <QtGui/QDesktopServices>

#include <glibmm.h>
#include <giomm.h>

using Platform::internal::GtkIntegration;

namespace Platform {
namespace File {

void UnsafeOpenUrl(const QString &url) {
	try {
		if (Gio::AppInfo::launch_default_for_uri(url.toStdString())) {
			return;
		}
	} catch (const Glib::Error &e) {
		LOG(("App Error: %1").arg(QString::fromStdString(e.what())));
	}

	QDesktopServices::openUrl(url);
}

void UnsafeOpenEmailLink(const QString &email) {
	UnsafeOpenUrl(qstr("mailto:") + email);
}

bool UnsafeShowOpenWith(const QString &filepath) {
	if (InFlatpak() || InSnap()) {
		return false;
	}

	if (const auto integration = GtkIntegration::Instance()) {
		return integration->showOpenWithDialog(filepath);
	}

	return false;
}

void UnsafeLaunch(const QString &filepath) {
	try {
		if (Gio::AppInfo::launch_default_for_uri(
			Glib::filename_to_uri(filepath.toStdString()))) {
			return;
		}
	} catch (const Glib::Error &e) {
		LOG(("App Error: %1").arg(QString::fromStdString(e.what())));
	}

	if (UnsafeShowOpenWith(filepath)) {
		return;
	}

	QDesktopServices::openUrl(QUrl::fromLocalFile(filepath));
}

} // namespace File

namespace FileDialog {

bool Get(
		QPointer<QWidget> parent,
		QStringList &files,
		QByteArray &remoteContent,
		const QString &caption,
		const QString &filter,
		::FileDialog::internal::Type type,
		QString startFile) {
	if (parent) {
		parent = parent->window();
	}
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	if (XDP::Use(type)) {
		return XDP::Get(
			parent,
			files,
			remoteContent,
			caption,
			filter,
			type,
			startFile);
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	if (const auto integration = GtkIntegration::Instance()) {
		if (integration->fileDialogSupported()
			&& integration->useFileDialog(type)) {
			return integration->getFileDialog(
				parent,
				files,
				remoteContent,
				caption,
				filter,
				type,
				startFile);
		}
	}
	return ::FileDialog::internal::GetDefault(
		parent,
		files,
		remoteContent,
		caption,
		filter,
		type,
		startFile);
}

} // namespace FileDialog
} // namespace Platform
