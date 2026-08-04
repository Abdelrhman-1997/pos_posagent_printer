#include <string>

#include "rapidjson/document.h"
#include "base64.hpp"
#include "messagesystem.h"
#include "dsoftruntime.h"
#include "printerroutingrequest.h"
#include "mainwindow.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

using namespace rapidjson;

namespace {
std::string jsonRpcResult(const std::string &id, bool success,
                          const QString &error = QString()) {
  QJsonObject root;
  root.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
  root.insert(QStringLiteral("id"), QString::fromStdString(id).toInt());
  root.insert(QStringLiteral("result"), success);
  if (!error.isEmpty())
    root.insert(QStringLiteral("error"), error);
  return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

void wakePrintProcessor() {
  if (MainWindow::active_window) {
    QMetaObject::invokeMethod(MainWindow::active_window, "refreshTimer",
                              Qt::QueuedConnection);
  }
}
}

namespace json {
const std::string getJsonStatusString(const char *request) {
  Document d;
  ParseResult ok = d.Parse(request);
  if (!ok) {
    fprintf(stderr, "JSON parse error");
    return "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":false}";
  }
  GlobalState::updatePrinterStatus();

  Value::ConstMemberIterator itr = d.FindMember("id");
  if (itr != d.MemberEnd()) {
    std::string ID = std::to_string(itr->value.IsInt() ? itr->value.GetInt() : -1);
    return "{\"jsonrpc\":\"2.0\",\"id\":" + ID +
           ",\"result\":{\"printer\":{\"status\":\"" +
           to_string(GlobalState::getPrinterStatus()) +
           "\",\"messages\":\"\"},\"scanner\":{\"status\":\"disconnected\",\"messages\":\"\"}}}";
  }
  return "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":false}";
}

const std::string getResultTrueString(const char *request) {
  Document d;
  ParseResult ok = d.Parse(request);
  if (!ok) {
    fprintf(stderr, "JSON parse error");
    return "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":false}";
  }

  Value::ConstMemberIterator itr = d.FindMember("id");
  if (itr != d.MemberEnd()) {
    std::string ID = std::to_string(itr->value.IsInt() ? itr->value.GetInt() : -1);
    return "{\"jsonrpc\":\"2.0\",\"id\":" + ID + ",\"result\":true}";
  }
  return "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":false}";
}

const std::string PrinterDefaultAction(const char *request) {
  Document d;
  ParseResult ok = d.Parse(request);
  if (!ok)
    return jsonRpcResult("0", false, QStringLiteral("Invalid JSON request."));

  std::string id = "0";
  auto idIt = d.FindMember("id");
  if (idIt != d.MemberEnd() && idIt->value.IsInt())
    id = std::to_string(idIt->value.GetInt());

  auto paramsIt = d.FindMember("params");
  if (paramsIt == d.MemberEnd() || !paramsIt->value.IsObject())
    return jsonRpcResult(id, false, QStringLiteral("Missing params object."));

  const Value &params = paramsIt->value;
  auto dataIt = params.FindMember("data");
  if (dataIt == params.MemberEnd() || !dataIt->value.IsObject())
    return jsonRpcResult(id, false, QStringLiteral("Missing data object."));

  QJsonObject data;
  const Value &rapidData = dataIt->value;
  for (auto member = rapidData.MemberBegin(); member != rapidData.MemberEnd(); ++member) {
    if (!member->name.IsString())
      continue;
    const QString key = QString::fromUtf8(member->name.GetString());
    if (member->value.IsString())
      data.insert(key, QString::fromUtf8(member->value.GetString()));
    else if (member->value.IsBool())
      data.insert(key, member->value.GetBool());
    else if (member->value.IsInt())
      data.insert(key, member->value.GetInt());
  }

  auto &runtime = DSoftRuntime::instance();
  const QString defaultCode =
      runtime.printerService().profileManager()->defaultProfileCode();
  const PrinterRoutingRequest routing =
      PrinterRoutingRequest::fromDataObject(data, defaultCode);
  if (!routing.valid)
    return jsonRpcResult(id, false, routing.error);

  quint64 sequence = 0;
  if (routing.action == QStringLiteral("print_receipt")) {
    const QByteArray decoded = QByteArray::fromBase64(
        routing.receiptBase64.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    if (decoded.isEmpty())
      return jsonRpcResult(id, false, QStringLiteral("Receipt base64 is invalid."));
    sequence = runtime.printerService().enqueueReceipt(routing.printerCode,
                                                       decoded);
  } else {
    sequence = runtime.printerService().enqueueCashDrawer(routing.printerCode);
  }

  if (sequence == 0)
    return jsonRpcResult(id, false, QStringLiteral("Could not enqueue print job."));

  wakePrintProcessor();
  return jsonRpcResult(id, true);
}
}
