
#pragma once
#include <QWidget>

class GUICenter : public QWidget
{
  Q_OBJECT
public:
  explicit GUICenter(QWidget* parent = nullptr);

public:
  void onOpenFile();
};
