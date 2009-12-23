/*
 * Copyright (C) 2009 Oliver Ney <oliver@dryder.de>
 *
 * This file is part of the OpenGL Overlay GUI Library (oGLui).
 *
 * oGLui is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with oGLui.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <core/oGLuiApplication.h>
#include <core/oGLuiVBoxLayout.h>
#include <core/oGLuiHBoxLayout.h>
#include <widgets/oGLuiBaseWidget.h>
#include <widgets/oGLuiButton.h>
#include <widgets/oGLuiLabel.h>
#include <widgets/oGLuiLineEdit.h>
#include <core/oGLuiPixmap.h>
#include <core/oGLuiSolidStyle.h>
#include <core/oGLuiConfig.h>

int main(int p_argc, char *p_argv[])
{
	oGLuiApplication app;

	oGLuiSolidStyle *style = new oGLuiSolidStyle();
	style->setBorderWidth(1);
	app.setStyle(style);

	// Base
	oGLuiBaseWidget base;
	base.setGeometry(oGLuiRect(0, 0, 150, 150));

	oGLuiVBoxLayout *main_layout = new oGLuiVBoxLayout();
	base.setLayout(main_layout);

	// Button
	oGLuiButton *button = new oGLuiButton("I'm a button!");
	main_layout->addWidget(button);

	// Label
	oGLuiLabel *label = new oGLuiLabel("Hahaha<sub>Chacka</sub>\nNERDMAN\n<b>Lala</b>Lalelu");
	label->setTextAlign(oGLui::Center);
	label->setMultiLine(true);
	main_layout->addWidget(label);

	// Input area
	oGLuiHBoxLayout *input_layout = new oGLuiHBoxLayout();
	main_layout->addElement(input_layout);

	// Line Edit
	oGLuiLineEdit *lineEdit = new oGLuiLineEdit("edit me!");
	input_layout->addWidget(lineEdit);

	app.processEvents();

	base.pixmap().toPNGFile("blahh.png");

	return 0;
}
