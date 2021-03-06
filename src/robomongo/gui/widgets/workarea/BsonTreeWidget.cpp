#include "robomongo/gui/widgets/workarea/BsonTreeWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/engine/JsonBuilder.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"


using namespace mongo;
namespace Robomongo
{
    BsonTreeWidget::BsonTreeWidget(MongoShell *shell, QWidget *parent) : QTreeWidget(parent),
        _shell(shell),
        _bus(AppRegistry::instance().bus()),
        _settingsManager(AppRegistry::instance().settingsManager())
    {
    #if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
    #endif

        GuiRegistry::instance().setAlternatingColor(this);

        QStringList colums;
        colums << "Key" << "Value" << "Type";
        setHeaderLabels(colums);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(0, QHeaderView::Stretch);
        header()->setSectionResizeMode(1, QHeaderView::Stretch);
        header()->setSectionResizeMode(2, QHeaderView::Stretch);
#endif
        setIndentation(15);

        //setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
        setContextMenuPolicy(Qt::DefaultContextMenu);

        _deleteDocumentAction = new QAction("Delete", this);
        connect(_deleteDocumentAction, SIGNAL(triggered()), SLOT(onDeleteDocument()));

        _editDocumentAction = new QAction("Edit", this);
        connect(_editDocumentAction, SIGNAL(triggered()), SLOT(onEditDocument()));

        _viewDocumentAction = new QAction("View", this);
        connect(_viewDocumentAction, SIGNAL(triggered()), SLOT(onViewDocument()));

        _insertDocumentAction = new QAction("Insert", this);
        connect(_insertDocumentAction, SIGNAL(triggered()), SLOT(onInsertDocument()));

        _copyValueAction = new QAction("Copy Value", this);
        connect(_copyValueAction, SIGNAL(triggered()), SLOT(onCopyDocument()));

        _expandRecursive = new QAction("Expand recursive", this);
         connect(_expandRecursive, SIGNAL(triggered()), SLOT(onExpandRecursive()));

    /*    _documentContextMenu = new QMenu(this);
        _documentContextMenu->addAction(_editDocumentAction);
        _documentContextMenu->addAction(_viewDocumentAction);
        _documentContextMenu->addSeparator();
        _documentContextMenu->addAction(_deleteDocumentAction);*/

        setStyleSheet(
            "QTreeWidget { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }"
        );
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(QHeaderView::Interactive);
#endif
        connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), SLOT(ui_itemExpanded(QTreeWidgetItem *)));
    }

    BsonTreeWidget::~BsonTreeWidget()
    {
        int h = 90;
    }

    void BsonTreeWidget::setDocuments(const QList<MongoDocumentPtr> &documents,
                                      const MongoQueryInfo &queryInfo /* = MongoQueryInfo() */)
    {
        _documents = documents;
        _queryInfo = queryInfo;

        setUpdatesEnabled(false);
        clear();

        BsonTreeItem *firstItem = NULL;

        QList<QTreeWidgetItem *> items;
        for (int i = 0; i < documents.count(); i++)
        {
            MongoDocumentPtr document = documents.at(i);

            BsonTreeItem *item = new BsonTreeItem(document, i);
            items.append(item);

            if (i == 0)
                firstItem = item;
        }

        addTopLevelItems(items);
        setUpdatesEnabled(true);

        if (firstItem)
        {
            firstItem->expand();
            firstItem->setExpanded(true);
        }
    }

    void BsonTreeWidget::ui_itemExpanded(QTreeWidgetItem *treeItem)
    {
        BsonTreeItem *item = static_cast<BsonTreeItem *>(treeItem);
        item->expand();

    /*	MongoDocumentIterator iterator(item->document());

        while(iterator.hasMore())
        {
            MongoElement_Pointer element = iterator.next();

            if (element->isSimpleType() || element->bsonElement().isNull())
            {
                QTreeWidgetItem *childItem = new QTreeWidgetItem;
                childItem->setText(0, element->fieldName());
                childItem->setText(1, element->stringValue());
                childItem->setIcon(0, getIcon(element));
                item->addChild(childItem);
            }
            else if (element->isDocument())
            {
                BsonTreeItem *newitem = new BsonTreeItem(element->asDocument(), element->isArray());

                if (item->isArray()) //is in array
                {
                    newitem->setText(0, QString("[%1]").arg(element->fieldName()));
                }
                else
                {
                    QString fieldName;

                    if (element->isArray())
                        fieldName = QString("%1 [%2]").arg(element->fieldName()).arg(element->bsonElement().Array().size());
                    else
                        fieldName = QString("%1 {..}").arg(element->fieldName());;

                    newitem->setText(0, fieldName);
                }

                newitem->setIcon(0, getIcon(element));
                newitem->setExpanded(true);
                newitem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
                item->addChild(newitem);
            }
        }
        */
    }

    QIcon BsonTreeWidget::getIcon(MongoElementPtr element)
    {
        if (element->isArray())
            return GuiRegistry::instance().bsonArrayIcon();

        if (element->isDocument())
            return GuiRegistry::instance().bsonObjectIcon();

        if (element->isSimpleType())
        {
    /*		if (element->fieldName() == "_id")
                return AppRegistry::instance().bsonIdIcon();*/

            if (element->isString())
                return GuiRegistry::instance().bsonStringIcon();

            if (element->bsonElement().type() == Timestamp || element->bsonElement().type() == Date)
                return GuiRegistry::instance().bsonDateTimeIcon();

            if (element->bsonElement().type() == NumberInt || element->bsonElement().type() == NumberLong)
                return GuiRegistry::instance().bsonIntegerIcon();

            if (element->bsonElement().type() == NumberDouble)
                return GuiRegistry::instance().bsonIntegerIcon();

            if (element->bsonElement().type() == Bool)
                return GuiRegistry::instance().bsonBooleanIcon();

            if (element->bsonElement().type() == BinData)
                return GuiRegistry::instance().bsonBinaryIcon();
        }

        if (element->bsonElement().type() == jstNULL)
            return GuiRegistry::instance().bsonNullIcon();

        return GuiRegistry::instance().circleIcon();
    }

    void BsonTreeWidget::contextMenuEvent(QContextMenuEvent *event)
    {
        QTreeWidgetItem *item = itemAt(event->pos());
        BsonTreeItem *documentItem = dynamic_cast<BsonTreeItem *>(item);

        bool isEditable = _queryInfo.isNull ? false : true;
        bool onItem = documentItem ? true : false;
        bool isSimple = documentItem ? (documentItem->isSimpleType() || documentItem->isUuidType()) : false;

        QMenu menu(this);
        if (onItem && isEditable) menu.addAction(_editDocumentAction);
        if (onItem)               menu.addAction(_viewDocumentAction);
        if (isEditable)           menu.addAction(_insertDocumentAction);
        if (onItem && isSimple)   menu.addSeparator();
        if (onItem && isSimple)   menu.addAction(_copyValueAction);
        if (onItem && isEditable) menu.addSeparator();
        if (onItem && isEditable) menu.addAction(_deleteDocumentAction);
        if (item&&item->childIndicatorPolicy()==QTreeWidgetItem::ShowIndicator)
        {
            menu.addAction(_expandRecursive);
        }
        QPoint menuPoint = mapToGlobal(event->pos());
        menuPoint.setY(menuPoint.y() + header()->height());
        menu.exec(menuPoint);
    }

    void BsonTreeWidget::resizeEvent(QResizeEvent *event)
    {
        QTreeWidget::resizeEvent(event);
        header()->resizeSections(QHeaderView::Stretch);
    }

    void BsonTreeWidget::onDeleteDocument()
    {
        if (_queryInfo.isNull)
            return;

        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        mongo::BSONElement id = obj.getField("_id");

        if (id.eoo()) {
            QMessageBox::warning(this, "Cannot delete", "Selected document doesn't have _id field. \n"
                                       "Maybe this is a system document that should be managed in a special way?");
            return;
        }

        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        // Ask user
        int answer = utils::questionDialog(this,"Delete","Document","%1 %2 with id:<br><b>%3</b>?",QString::fromStdString(id.toString(false)));

        if (answer != QMessageBox::Yes)
            return ;

        _shell->server()->removeDocuments(query, _queryInfo.databaseName, _queryInfo.collectionName);
        _shell->query(0, _queryInfo);
    }

    void BsonTreeWidget::onEditDocument()
    {
        if (_queryInfo.isNull)
            return;

        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        std::string str = JsonBuilder::jsonString(obj, mongo::TenGen, 1, _settingsManager->uuidEncoding());
        QString json = QString::fromUtf8(str.data());

        DocumentTextEditor editor(_queryInfo.serverAddress,
                                  _queryInfo.databaseName,
                                  _queryInfo.collectionName,
                                  json);

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();
        activateWindow();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            _bus->subscribe(this, InsertDocumentResponse::Type);
            _shell->server()->saveDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
        }
    }

    void BsonTreeWidget::onViewDocument()
    {
        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        std::string str = JsonBuilder::jsonString(obj, mongo::TenGen, 1, _settingsManager->uuidEncoding());
        QString json = QString::fromUtf8(str.data());

        QString server = _queryInfo.isNull ? "" : _queryInfo.serverAddress;
        QString database = _queryInfo.isNull ? "" : _queryInfo.databaseName;
        QString collection = _queryInfo.isNull ? "" : _queryInfo.collectionName;

        DocumentTextEditor *editor = new DocumentTextEditor(server,
                                  database,
                                  collection,
                                  json, true, this);

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void BsonTreeWidget::onInsertDocument()
    {
        if (_queryInfo.isNull)
            return;

        DocumentTextEditor editor(_queryInfo.serverAddress,
                                  _queryInfo.databaseName,
                                  _queryInfo.collectionName,
                                  "{\n    \n}");

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");
        int result = editor.exec();
        activateWindow();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            _shell->server()->insertDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
            _shell->query(0, _queryInfo);
        }
    }

    void BsonTreeWidget::onCopyDocument()
    {
        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        MongoElementPtr element = documentItem->element();

        if (!element->isSimpleType() && !element->isUuidType())
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(element->stringValue());
    }
    void BsonTreeWidget::expandNode(QTreeWidgetItem *item)
    {
        if(item)
        {
            ui_itemExpanded(item);
            item->setExpanded(true);
            for(int i = 0;i<item->childCount();++i)
            {
                QTreeWidgetItem *tritem = item->child(i);
                if(tritem&&tritem->childIndicatorPolicy()==QTreeWidgetItem::ShowIndicator)
                {
                    expandNode(tritem);
                }
            }
        }
    }
    void BsonTreeWidget::onExpandRecursive()
    {
        expandNode(currentItem());
    }
    void BsonTreeWidget::handle(InsertDocumentResponse *event)
    {
        _bus->unsubscibe(this);
        _shell->query(0, _queryInfo);
    }

    /**
     * @returns selected BsonTreeItem, or NULL otherwise
     */
    BsonTreeItem *BsonTreeWidget::selectedBsonTreeItem()
    {
        QList<QTreeWidgetItem*> items = selectedItems();

        if (items.count() != 1)
            return NULL;

        QTreeWidgetItem *item = items[0];

        if (!item)
            return NULL;

        return dynamic_cast<BsonTreeItem *>(item);
    }

    QString BsonTreeWidget::selectedJson()
    {
        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return "";

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        std::string str = JsonBuilder::jsonString(obj, mongo::TenGen, 1, _settingsManager->uuidEncoding());
        QString json = QString::fromUtf8(str.data());
        return json;
    }
}
