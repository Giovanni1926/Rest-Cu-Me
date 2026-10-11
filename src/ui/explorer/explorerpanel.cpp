#include "explorerpanel.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QStyle>
#include <QIcon>
#include <QPalette>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include "../widgets/appicons.h"
#include "../../flow/flowgraph.h"

namespace rcm {

static QString nodeLabel(const ExplorerNode& node) {
    QString kind;
    if (node.kind == FlowNode::KIND_REST) kind = QStringLiteral("REST");
    else if (node.kind == FlowNode::KIND_SOAP) kind = QStringLiteral("SOAP");
    else kind = QStringLiteral("Variabili");
    return node.title.isEmpty() ? kind : kind + QStringLiteral(" \u00b7 ") + node.title;
}

ExplorerPanel::ExplorerPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setRootIsDecorated(true);
    layout->addWidget(tree_);

    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        const QString type = item->data(0, Qt::UserRole).toString();
        const QString id = item->data(0, Qt::UserRole + 1).toString();
        if (type == QStringLiteral("file")) {
            if (fileActivated) fileActivated(id);
        } else if (type == QStringLiteral("node")) {
            if (nodeActivated) nodeActivated(id);
        } else if (type == QStringLiteral("group")) {
            if (groupActivated) groupActivated(id);
        }
    });
}

void ExplorerPanel::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = tree_->itemAt(event->pos());
    QString path;
    if (item) {
        QTreeWidgetItem* top = item;
        while (top->parent()) top = top->parent();
        path = top->data(0, Qt::UserRole + 1).toString();
    }

    QMenu menu(this);
    QMenu* newMenu = menu.addMenu(QStringLiteral("Nuovo"));
    QAction* restAction = newMenu->addAction(QStringLiteral("Chiamata REST..."));
    QAction* soapAction = newMenu->addAction(QStringLiteral("Chiamata SOAP..."));
    QAction* varsAction = newMenu->addAction(QStringLiteral("Nodo variabili..."));
    QAction* groupAction = newMenu->addAction(QStringLiteral("Nuova collection..."));

    QAction* chosen = menu.exec(event->globalPos());
    if (!chosen || !addRequested) return;
    if (chosen == restAction) addRequested(path, AddRest);
    else if (chosen == soapAction) addRequested(path, AddSoap);
    else if (chosen == varsAction) addRequested(path, AddVariables);
    else if (chosen == groupAction) addRequested(path, AddGroup);
}

void ExplorerPanel::refresh(const std::vector<ExplorerFile>& files, const QString& currentPath,
                            const QString& selectedNodeId, const QString& selectedGroupId) {
    tree_->clear();
    const QColor iconColor = palette().color(QPalette::Text);

    for (const auto& file : files) {
        QTreeWidgetItem* fileItem = new QTreeWidgetItem(tree_);
        fileItem->setText(0, file.name);
        fileItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        fileItem->setData(0, Qt::UserRole, QStringLiteral("file"));
        fileItem->setData(0, Qt::UserRole + 1, file.path);

        for (const auto& group : file.groups) {
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(fileItem);
            groupItem->setText(0, group.name);
            groupItem->setIcon(0, collectionIcon(iconColor));
            groupItem->setData(0, Qt::UserRole, QStringLiteral("group"));
            groupItem->setData(0, Qt::UserRole + 1, group.id);
            for (const auto& node : group.nodes) {
                QTreeWidgetItem* nodeItem = new QTreeWidgetItem(groupItem);
                nodeItem->setText(0, nodeLabel(node));
                nodeItem->setData(0, Qt::UserRole, QStringLiteral("node"));
                nodeItem->setData(0, Qt::UserRole + 1, node.id);
                if (node.kind == FlowNode::KIND_SOAP) nodeItem->setIcon(0, soapIcon(iconColor));
                else if (node.kind == FlowNode::KIND_VARIABLES) nodeItem->setIcon(0, dollarIcon(iconColor));
                else nodeItem->setIcon(0, globeIcon(iconColor));
            }
        }

        for (const auto& node : file.looseNodes) {
            QTreeWidgetItem* nodeItem = new QTreeWidgetItem(fileItem);
            nodeItem->setText(0, nodeLabel(node));
            nodeItem->setData(0, Qt::UserRole, QStringLiteral("node"));
            nodeItem->setData(0, Qt::UserRole + 1, node.id);
            if (node.kind == FlowNode::KIND_SOAP) nodeItem->setIcon(0, soapIcon(iconColor));
            else if (node.kind == FlowNode::KIND_VARIABLES) nodeItem->setIcon(0, dollarIcon(iconColor));
            else nodeItem->setIcon(0, globeIcon(iconColor));
        }

        fileItem->setExpanded(true);
    }

    tree_->expandAll();

    auto selectById = [this](const QString& targetType, const QString& targetId) {
        if (targetId.isEmpty()) return;
        const int fileCount = tree_->topLevelItemCount();
        for (int i = 0; i < fileCount; ++i) {
            QTreeWidgetItem* fileItem = tree_->topLevelItem(i);
            const int childCount = fileItem->childCount();
            for (int j = 0; j < childCount; ++j) {
                QTreeWidgetItem* child = fileItem->child(j);
                const QString type = child->data(0, Qt::UserRole).toString();
                const QString id = child->data(0, Qt::UserRole + 1).toString();
                if (type == targetType && id == targetId) {
                    tree_->setCurrentItem(child);
                    return;
                }
                const int grandCount = child->childCount();
                for (int k = 0; k < grandCount; ++k) {
                    QTreeWidgetItem* grand = child->child(k);
                    const QString gtype = grand->data(0, Qt::UserRole).toString();
                    const QString gid = grand->data(0, Qt::UserRole + 1).toString();
                    if (gtype == targetType && gid == targetId) {
                        tree_->setCurrentItem(grand);
                        return;
                    }
                }
            }
        }
    };

    if (!selectedNodeId.isEmpty()) selectById(QStringLiteral("node"), selectedNodeId);
    else if (!selectedGroupId.isEmpty()) selectById(QStringLiteral("group"), selectedGroupId);
}

} // namespace rcm
