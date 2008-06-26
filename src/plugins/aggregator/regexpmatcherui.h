#ifndef REGEXPMATCHERUI_H
#define REGEXPMATCHERUI_H
#include <QDialog>
#include "ui_regexpmatcherui.h"

class RegexpMatcherUi : public QDialog
{
	Q_OBJECT

	Ui::RegexpMatcherUi Ui_;
	RegexpMatcherUi ();
public:
	virtual ~RegexpMatcherUi ();
	static RegexpMatcherUi& Instance ();
	void Release ();
private slots:
	void on_AddRegexpButton__released ();
	void on_ModifyRegexpButton__released ();
	void on_RemoveRegexpButton__released ();
};

#endif

