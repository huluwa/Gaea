/*
 *  Copyright Beijing 58 Information Technology Co.,Ltd.
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
/*
 * ProxyStandard.cpp
   *
 * Created on: 2011-7-12
 * Author: Service Platform Architecture Team (spat@58.com)
 */

#include "ProxyStandard.h"
#include "Parameter.h"
#include "ServiceProxy.h"
#include "Log.h"
#include <string>
#include <errno.h>
namespace gaea {
ThreadPool* ProxyStandard::tp = NULL;
pthread_mutex_t ProxyStandard::mutex = PTHREAD_MUTEX_INITIALIZER;
ProxyStandard::ProxyStandard(std::string serviceName, char *lookup) {
	this->serviceName = serviceName;
	this->lookup = lookup;
	if (tp == NULL) {
		pthread_mutex_lock(&mutex);
		if (tp == NULL) {
			tp = threadpool_create(20, 20000);
		}
		pthread_mutex_unlock(&mutex);
	}
}
void *ProxyStandard::invoke(char *methodName, Parameter **para, int paraLen) {
	int outPara[paraLen];
	int outParaLen = 0;
	int index = 0;
	for (int i = 0; i < paraLen; ++i) {
		if ((*(para + i))->getParaType() == PARA_OUT) {
			outPara[index] = i;
			++index;
		}
	}
	ServiceProxy *proxy = ServiceProxy::getProxy(serviceName);
	InvokeResult *result = proxy->invoke(lookup, methodName, para, paraLen);
	if (result && result->getOutPara()) {
		for (int i = 0; i < outParaLen; ++i) {
			memcpy(*(para + outPara[i]), (void**)result->getOutPara()->data + i, sizeof(void*));
		}
	}
	void *r = result->getResult();
	delete result;
	return r;
}
int ProxyStandard::asyncInvoke(char *methodName, Parameter **para, int paraLen, call_back_fun_ callBackFun, void *context) {
	ProxStandardSession *pss = new ProxStandardSession(methodName, para, paraLen, callBackFun, context, this);
	return threadpool_add_task(tp, asyncInvoke, pss);
}
void ProxyStandard::asyncInvoke(void *data) {
	ProxStandardSession *pss = (ProxStandardSession*) data;
	void *retData = NULL;
	try {
		retData = pss->getPs()->invoke(pss->getMethodName(), pss->getPara(), pss->getParaLen());
	} catch (std::exception &e) {
		gaeaLog(GAEA_WARNING, e.what());
		if (errno == 0) {
			errno = -3;
		}
		pss->getCallBackFun()(errno, NULL, pss->getContext());
		delete pss;
		return;
	}
	pss->getCallBackFun()(0, retData, pss->getContext());
	delete pss;
}
ProxyStandard::~ProxyStandard() {
}

call_back_fun_ ProxStandardSession::getCallBackFun() const {
	return callBackFun;
}

void *ProxStandardSession::getContext() const {
	return context;
}

char *ProxStandardSession::getMethodName() const {
	return methodName;
}

Parameter **ProxStandardSession::getPara() const {
	return para;
}

int ProxStandardSession::getParaLen() const {
	return paraLen;
}

ProxyStandard *ProxStandardSession::getPs() const {
	return ps;
}
}
/* namespace gaea */
