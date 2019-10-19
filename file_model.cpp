#include "file_model.h"

#include <QMimeData>
#include <QDebug>

FileNode::FileNode(const QString& filename):
    QStandardItem(filename) {

}

void FileNode::appendChildren(const QVector<QStringList>& fileList) {
    int len = fileList.length();
    if (len) clearChildren();
    for (int i = 0;i < len;i++) {
        FileNode* node = new FileNode(fileList[i][0]);

        appendRow(node);
        setChild(node->index().row(), 1, new FileNode(fileList[i][1]));
        setChild(node->index().row(), 2, new FileNode(fileList[i][2]));
        setChild(node->index().row(), 3, new FileNode(fileList[i][3]));
        setChild(node->index().row(), 4, new FileNode(fileList[i][4]));
        if (fileList[i][3][0] == 'd') {
            appendFakeNode(node);
        }
    }
}

FileNode::Type FileNode::getType() {
    QStandardItem* _this = this;
    if (!_this->parent()) return DIR; // 根目录
    QModelIndex index = model()->indexFromItem(this).siblingAtColumn(3);
    QString text = model()->itemFromIndex(index)->text();
    if (text[0] == 'd') return DIR;
    else if (text.isEmpty()) {
        return EMPTY;
    }
    else return FILE;
}

QString FileNode::getPath() {
    QStandardItem* p = this;
    if (this->getType() == FILE) {
        p = p->parent();
    }
    QString path = p->text();
    p = p->parent();
    while (p) {
        int len = p->text().length();
        if (len && p->text()[len-1] == '/') {
            path = p->text()+path;
        }
        else path = p->text()+'/'+path;
        p = p->parent();
    }
    return path;
}

void FileNode::clearChildren() {
    removeRows(0, rowCount());
}

QVector<QStringList> FileNode::parseFileListStr(const QString& fileListStr) {
    // QStringList infoList = fileListStr.split(QRegExp("[\r][\n]"));
    QStringList infoList = fileListStr.split("\r\n", QString::SkipEmptyParts);
    QRegExp regex("[ ]+");
    QVector<QStringList> fileList;
    QString filename, size, modifyDate, authority, ownerNGroup;
    int len = infoList.length();
    for (int i = 0;i < len;i++) {
        filename = infoList[i].section(' ', 8, -1, QString::SectionSkipEmpty);
        size = infoList[i].section(' ', 4, 4, QString::SectionSkipEmpty);
        modifyDate = infoList[i].section(' ', 5, 7, QString::SectionSkipEmpty);
        authority = infoList[i].section(' ', 0, 0, QString::SectionSkipEmpty);
        ownerNGroup = infoList[i].section(' ', 2, 2, QString::SectionSkipEmpty)+" "
                      +infoList[i].section(' ', 3, 3, QString::SectionSkipEmpty);

        if (!filename.isEmpty()) {
            QStringList file;
            file << filename << size << modifyDate << authority << ownerNGroup;
            fileList.append(file);
        }
    }
    return fileList;
}

void FileNode::appendFakeNode(FileNode* node) {
    if (node->rowCount()) return;
    FileNode* fakeNode = new FileNode("");
    node->appendRow(fakeNode);
    node->setChild(fakeNode->index().row(), 1, new FileNode(""));
    node->setChild(fakeNode->index().row(), 2, new FileNode(""));
    node->setChild(fakeNode->index().row(), 3, new FileNode(""));
    node->setChild(fakeNode->index().row(), 4, new FileNode(""));
}

FileNode* FileNode::findNodeByPath(FileNode* root, const QString& path) {
    int index = path.indexOf(root->text());
    if (index) return nullptr;
    QStringList dirList = path.mid(root->text().length()).split('/', QString::SkipEmptyParts);
    int len = dirList.length();
    QStandardItem* p = root;
    for (int i = 0;i < len;i++) {
        int rowCount = p->rowCount();
        bool found = false;
        for (int j = 0;j < rowCount;j++) {
            if (p->child(j)->text() == dirList[i]) {
                found = true;
                p = p->child(j);
                break;
            }
        }
        if (!found) return nullptr;
    }
    FileNode* node = dynamic_cast<FileNode*>(p);
    if (node) return node;
    else return nullptr;
}

FileModel::FileModel(QObject* parent, const QString& rootPath):
    QStandardItemModel(parent) {
    QStringList header;
    header << "文件名" << "文件大小" << "最近修改" << "权限" << "所有者/组";
    setHorizontalHeaderLabels(header);

    root = new FileNode(rootPath);
    appendRow(root);
    FileNode::appendFakeNode(root);
}

FileNode* FileModel::getRoot() {
    return root;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags = flags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    return flags;
}

QMimeData* FileModel::mimeData(const QModelIndexList &indexes) const {
    if (indexes.count() <= 0) return 0;

    QMimeData *data = new QMimeData;
    FileNode* node = dynamic_cast<FileNode*>(itemFromIndex(indexes.at(0)));
    if (!node) return 0;

    if (node->getType() == FileNode::FILE) {
        // 仅允许拖拽文件
        data->setData("path", node->getPath().toLocal8Bit());
        data->setData("filename", node->text().toLocal8Bit());
    }
    else return 0;

    return data;
}

bool FileModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    QModelIndex index = parent.siblingAtColumn(0);
    FileNode* node = dynamic_cast<FileNode*>(itemFromIndex(index));
    if (!node) return false;

    QString path = QString::fromLocal8Bit(data->data("path"));
    QString filename = QString::fromLocal8Bit(data->data("filename"));
    qDebug() << "path:" << path << "filename:" << filename;

    qDebug() << node->getPath();

    emit transfer(path, filename, node->getPath());
    return true;
}

QStringList FileModel::mimeTypes() const {
    QStringList types;
    types << "path" << "filename";
    return types;
}


