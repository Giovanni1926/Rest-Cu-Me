#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <functional>
#include <vector>
#include <QString>

namespace rcm {

enum AddKind { AddRest, AddSoap, AddVariables, AddGroup };

struct ExplorerNode {
    QString id;
    QString kind;
    QString title;
};

struct ExplorerGroup {
    QString id;
    QString name;
    std::vector<ExplorerNode> nodes;
};

struct ExplorerFile {
    QString path;
    QString name;
    std::vector<ExplorerGroup> groups;
    std::vector<ExplorerNode> looseNodes;
};

class ExplorerPanel : public QWidget {
public:
    explicit ExplorerPanel(QWidget* parent = nullptr);

    void refresh(const std::vector<ExplorerFile>& files, const QString& currentPath,
                 const QString& selectedNodeId, const QString& selectedGroupId);

    std::function<void(const QString&)> fileActivated;
    std::function<void(const QString&)> nodeActivated;
    std::function<void(const QString&)> groupActivated;
    std::function<void(const QString&, int)> addRequested;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QTreeWidget* tree_ = nullptr;
};

} // namespace rcm
