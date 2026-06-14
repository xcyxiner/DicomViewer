
#pragma once
#include <QMainWindow>

#include "FileMenu.h"
#include "qtmetamacros.h"
#include "qwidget.h"

class GUIWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit GUIWindow(QWidget* parent = nullptr);

public:
  void setContent(QWidget* t_widget);
  void openFile();

private:
  void createMenu();

private:
  QWidget* m_childWidget = {};

  FileMenu* m_fileMenu;
};
