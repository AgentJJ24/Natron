/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"
#include "KnobPrivate.h"

#include <sstream> // stringstream

#include "Engine/KnobItemsTable.h"

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

/**
 * @brief Given the string str, returns the position of the character "closingChar" corresponding to the "openingChar" at the given openingParenthesisPos
 * For example if the str is "((lala)+toto)" and we want to get the character matching the first '(', this function will return the last parenthesis in the string
 * @return The position of the matching closing character or std::string::npos if not found
 **/
static std::size_t
getMatchingParenthesisPosition(std::size_t openingParenthesisPos,
                               char openingChar,
                               char closingChar,
                               const std::string& str)
{
    assert(openingParenthesisPos < str.size() && str.at(openingParenthesisPos) == openingChar);

    int noOpeningParenthesisFound = 0;
    int i = openingParenthesisPos + 1;

    while ( i < (int)str.size() ) {
        if (str.at(i) == closingChar) {
            if (noOpeningParenthesisFound == 0) {
                break;
            } else {
                --noOpeningParenthesisFound;
            }
        } else if (str.at(i) == openingChar) {
            ++noOpeningParenthesisFound;
        }
        ++i;
    }
    if ( i >= (int)str.size() ) {
        return std::string::npos;
    }

    return i;
} // getMatchingParenthesisPosition

/**
 * @brief Given a string str, assume that the content of the string between startParenthesis and endParenthesis 
 * is a well-formed Python signature with comma separated arguments list. The list of arguments is stored in params.
 **/
static void
extractParameters(std::size_t startParenthesis,
                  std::size_t endParenthesis,
                  const std::string& str,
                  std::vector<std::string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;

    while (i < endParenthesis || insideParenthesis < 0) {
        std::string curParam;
        if (str.at(i) == '(') {
            ++insideParenthesis;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
        }
        while ( i < str.size() && (str.at(i) != ',' || insideParenthesis > 0) ) {
            curParam.push_back( str.at(i) );
            ++i;
            if (str.at(i) == '(') {
                ++insideParenthesis;
            } else if (str.at(i) == ')') {
                if (insideParenthesis > 0) {
                    --insideParenthesis;
                } else {
                    break;
                }
            }
        }
        params->push_back(curParam);
    }
} // extractParameters

/**
 * @brief Given the string str, tries to find the given function name "token" string starting from inputPos.
 * @param fromDim The dimension in the knob on which the function is called
 * @param fromViewName The name of the view in the knob on which the function is called
 * @param dimensionParamPos Indicate the index (0-based) of the "dimension" argument in the function given by token
 * @param viewParamPos Indicate the index (0-based) of the "view" argument in the function given by token
 * E.g: In the function get(frame, dimension, view), the dimension parameter index is 1.
 * @param returnsTuple If true, indicates that the function given in "token" is expected to return a tuple
 * @param output[out] The script to execute to register this parameter as a dependency of the other parameter in the expression
 * @return true on success, false otherwise
 * Note: this function can throw an exception if the expression is incorrect
 **/
static bool
parseTokenFrom(int fromDim,
               const std::string& fromViewName,
               int dimensionParamPos,
               int viewParamPos,
               bool returnsTuple,
               const std::string& str,
               const std::string& token,
               std::size_t inputPos,
               std::size_t *tokenStart,
               std::string* output)
{
    std::size_t pos;

    // Find the opening parenthesis after the token start
    bool foundMatchingToken = false;
    while (!foundMatchingToken) {
        pos = str.find(token, inputPos);
        if (pos == std::string::npos) {
            return false;
        }

        *tokenStart = pos;
        pos += token.size();

        ///Find nearest opening parenthesis
        for (; pos < str.size(); ++pos) {
            if (str.at(pos) == '(') {
                foundMatchingToken = true;
                break;
            } else if (str.at(pos) != ' ') {
                //We didn't find a good token
                break;
            }
        }

        if ( pos >= str.size() ) {
            throw std::invalid_argument("Invalid expr");
        }

        if (!foundMatchingToken) {
            inputPos = pos;
        }
    }

    // Get the closing parenthesis for the function
    std::size_t endingParenthesis = getMatchingParenthesisPosition(pos, '(', ')',  str);
    if (endingParenthesis == std::string::npos) {
        throw std::invalid_argument("Invalid expr");
    }

    // Extract parameters in between the 2 parenthesis
    std::vector<std::string> params;
    extractParameters(pos, endingParenthesis, str, &params);


    bool gotViewParam = viewParamPos != -1 && (viewParamPos < (int)params.size() && !params.empty());
    bool gotDimensionParam = dimensionParamPos != -1 && (dimensionParamPos < (int)params.size() && !params.empty());


    if (!returnsTuple) {
        // Before Natron 2.2 the View parameter of the get(view) function did not exist, hence loading
        // an old expression may use the old get() function without view. If we do not find any view
        // parameter, assume the view is "Main" by default.
        if (!gotViewParam && viewParamPos != -1) {
            std::vector<std::string>::iterator it = params.begin();
            if (viewParamPos >= (int)params.size()) {
                it = params.end();
            } else {
                std::advance(it, viewParamPos);
            }
            params.insert(it, "Main");
        }
        if (!gotDimensionParam && dimensionParamPos != -1) {
            std::vector<std::string>::iterator it = params.begin();
            if (dimensionParamPos >= (int)params.size()) {
                it = params.end();
            } else {
                std::advance(it, dimensionParamPos);
            }
            params.insert(it, "0");
        }
    } else {
        assert(dimensionParamPos == -1 && !gotDimensionParam);

        // If the function returns a tuple like get()[dimension],
        // also find the parameter in-between the tuple brackets
        //try to find the tuple
        std::size_t it = endingParenthesis + 1;
        while (it < str.size() && str.at(it) == ' ') {
            ++it;
        }
        if ( ( it < str.size() ) && (str.at(it) == '[') ) {
            ///we found a tuple
            std::size_t endingBracket = getMatchingParenthesisPosition(it, '[', ']',  str);
            if (endingBracket == std::string::npos) {
                throw std::invalid_argument("Invalid expr");
            }
            params.push_back( str.substr(it + 1, endingBracket - it - 1) );
        } else {
            // No parameters in the tuple: assume this is all dimensions.
            params.push_back("-1");
        }
        dimensionParamPos = 1;
        gotDimensionParam = true;

        // Before Natron 2.2 the View parameter of the get(view) function did not exist, hence loading
        // an old expression may use the old get() function without view. If we do not find any view
        // parameter, assume the view is "Main" by default.
        if (params.size() == 1 && viewParamPos >= 0) {
            if (viewParamPos < dimensionParamPos) {
                params.insert(params.begin(), "Main");
            } else {
                params.push_back("Main");
            }
        }

    }

    if ( (dimensionParamPos < 0) || ( (int)params.size() <= dimensionParamPos ) ) {
        throw std::invalid_argument("Invalid expr");
    }

    if ( (viewParamPos < 0) || ( (int)params.size() <= viewParamPos ) ) {
        throw std::invalid_argument("Invalid expr");
    }

    std::string toInsert;
    {
        std::stringstream ss;
        /*
         When replacing the getValue() (or similar function) call by addAsDepdendencyOf
         the parameter prefixing the addAsDepdendencyOf will register itself its dimension params[dimensionParamPos] as a dependency of the expression
         at the "fromDim" dimension of thisParam
         */
        ss << ".addAsDependencyOf(thisParam, " << fromDim << "," <<
        params[dimensionParamPos] << ", \"" << fromViewName << "\", \"" <<
        params[viewParamPos] << "\")\n";
        toInsert = ss.str();
    }

    // Find the Python attribute which called the function "token"
    if ( (*tokenStart < 2) || (str[*tokenStart - 1] != '.') ) {
        throw std::invalid_argument("Invalid expr");
    }

    std::locale loc;
    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if ( std::isspace(str[i], loc) ||
            ( str[i] == '=') ||
            ( str[i] == '\n') ||
            ( str[i] == '\t') ||
            ( str[i] == '+') ||
            ( str[i] == '-') ||
            ( str[i] == '*') ||
            ( str[i] == '/') ||
            ( str[i] == '%') ||
            ( ( str[i] == '(') && !nClosingParenthesisMet ) ) {
            break;
        }
        --i;
    }
    ++i;

    // This is the name of the Python attribute that called "token"
    std::string varName = str.substr(i, *tokenStart - 1 - i);
    output->append(varName + toInsert);

    //assert(*tokenSize > 0);
    return true;
} // parseTokenFrom

/**
 * @brief Calls parseTokenFrom until all tokens in the expression have been found.
 * @param outputScript[out] The script to execute in output to register this parameter as a dependency of the other in the expression
 * @return true on success, false otherwise
 * Note: this function can throw an exception if the expression is incorrect
 **/
static bool
extractAllOcurrences(const std::string& str,
                     const std::string& token,
                     bool returnsTuple,
                     int dimensionParamPos,
                     int viewParamPos,
                     int fromDim,
                     const std::string& fromViewName,
                     std::string *outputScript)
{
    std::size_t tokenStart = 0;
    bool couldFindToken;

    do {
        try {
            couldFindToken = parseTokenFrom(fromDim, fromViewName, dimensionParamPos, viewParamPos, returnsTuple, str, token, tokenStart == 0 ? 0 : tokenStart + 1, &tokenStart, outputScript);
        } catch (...) {
            return false;
        }
    } while (couldFindToken);

    return true;
}
NATRON_NAMESPACE_ANONYMOUS_EXIT

std::string
KnobHelperPrivate::getReachablePythonAttributesForExpression(bool addTab,
                                          DimIdx dimension,
                                          ViewIdx /*view*/)
{
    KnobHolderPtr h = holder.lock();
    assert(h);
    if (!h) {
        throw std::runtime_error("This parameter cannot have an expression");
    }


    NodePtr node;
    EffectInstancePtr effect = toEffectInstance(h);
    KnobTableItemPtr tableItem = toKnobTableItem(h);
    if (effect) {
        node = effect->getNode();
    } else if (tableItem) {
        KnobItemsTablePtr model = tableItem->getModel();
        if (model) {
            node = model->getNode();
        }
    }

    assert(node);
    if (!node) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodeCollectionPtr collection = node->getGroup();
    if (!collection) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    std::string appID = node->getApp()->getAppIDString();
    std::string tabStr = addTab ? "    " : "";
    std::stringstream ss;
    if (appID != "app") {
        ss << tabStr << "app = " << appID << "\n";
    }


    //Define all nodes reachable through expressions in the scope


    // Define all nodes in the same group reachable by their bare script-name
    NodesList siblings = collection->getNodes();
    std::string collectionScriptName;
    if (isParentGrp) {
        collectionScriptName = isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        collectionScriptName = appID;
    }
    for (NodesList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if ((*it)->isActivated()) {
            std::string scriptName = (*it)->getScriptName_mt_safe();
            std::string fullName = appID + "." + (*it)->getFullyQualifiedName();

            // Do not fail the expression if the attribute do not exist to Python, use hasattr
            ss << tabStr << "if hasattr(";
            if (isParentGrp) {
                ss << appID << ".";
            }
            ss << collectionScriptName << ",\"" <<  scriptName << "\"):\n";

            ss << tabStr << "    " << scriptName << " = " << fullName << "\n";
        }
    }

    // Define thisGroup
    if (isParentGrp) {
        ss << tabStr << "thisGroup = " << appID << "." << collectionScriptName << "\n";
    } else {
        ss << tabStr << "thisGroup = " << appID << "\n";
    }


    // Define thisNode
    ss << tabStr << "thisNode = " << node->getScriptName_mt_safe() <<  "\n";

    if (tableItem) {
        const std::string& tablePythonPrefix = tableItem->getModel()->getPythonPrefix();
        ss << tabStr << "thisItem = thisNode." << tablePythonPrefix << tableItem->getFullyQualifiedName() << "\n";
    }

    // Define thisParam
    if (tableItem) {
        ss << tabStr << "thisParam = thisItem." << name << "\n";
    } else {
        ss << tabStr << "thisParam = thisNode." << name << "\n";
    }

    // Define dimension variable
    ss << tabStr << "dimension = " << dimension << "\n";

    // Declare common functions
    ss << tabStr << "random = thisParam.random\n";
    ss << tabStr << "randomInt = thisParam.randomInt\n";
    ss << tabStr << "curve = thisParam.curve\n";


    return ss.str();
} // KnobHelperPrivate::declarePythonVariables

void
KnobHelperPrivate::parseListenersFromExpression(DimIdx dimension, ViewIdx view)
{
    assert(dimension >= 0 && dimension < (int)expressions.size());

    // Extract pointers to knobs referred to by the expression
    // Our heuristic is quite simple: we replace in the python code all calls to:
    // - getValue
    // - getValueAtTime
    // - getDerivativeAtTime
    // - getIntegrateFromTimeToTime
    // - get
    // And replace them by addAsDependencyOf(thisParam) which will register the parameters as a dependency of this parameter

    std::string viewName = holder.lock()->getApp()->getProject()->getViewName(view);
    assert(!viewName.empty());
    if (viewName.empty()) {
        viewName = "Main";
    }
    std::string expressionCopy;

    {
        QMutexLocker k(&expressionMutex);
        ExprPerViewMap::const_iterator foundView = expressions[dimension].find(view);
        if (foundView == expressions[dimension].end()) {
            return;
        }
        expressionCopy = foundView->second.originalExpression;
    }

    // Extract parameters that call the following functions
    std::string listenersRegistrationScript;
    if  ( !extractAllOcurrences(expressionCopy, "getValue", false, 0, 1, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getValueAtTime", false, 1, 2, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getDerivativeAtTime", false, 1, 2, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getIntegrateFromTimeToTime", false, 2, 3, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "get", true, -1, 0, dimension, viewName, &listenersRegistrationScript) ) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): failed!");
        return;
    }

    // Declare all attributes that may be reached through this expression
    std::string declarations = getReachablePythonAttributesForExpression(false, dimension, view);

    // Make up the internal expression
    std::stringstream ss;
    ss << "frame=0\n";
    ss << "view=\"Main\"\n";
    ss << declarations << '\n';
    ss << expressionCopy << '\n';
    ss << listenersRegistrationScript;
    std::string script = ss.str();

    // Execute the expression: this will register listeners
    std::string error;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, NULL);
    if ( !error.empty() ) {
        qDebug() << error.c_str();
    }

    // This should always succeed, otherwise there is a bug in this function.
    assert(ok);
    if (!ok) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): interpretPythonScript(" + script + ") failed!");
    }
} // KnobHelperPrivate::parseListenersFromExpression

std::string
KnobHelper::validateExpression(const std::string& expression,
                               DimIdx dimension,
                               ViewIdx view,
                               bool hasRetVariable,
                               std::string* resultAsString)
{

#ifdef NATRON_RUN_WITHOUT_PYTHON
    throw std::invalid_argument("NATRON_RUN_WITHOUT_PYTHON is defined");
#endif
    PythonGILLocker pgl;

    if ( expression.empty() ) {
        throw std::invalid_argument("Empty expression");;
    }


    std::string exprCpy = expression;

    //if !hasRetVariable the expression is expected to be single-line
    if (!hasRetVariable) {
        std::size_t foundNewLine = expression.find("\n");
        if (foundNewLine != std::string::npos) {
            throw std::invalid_argument(tr("unexpected new line character \'\\n\'").toStdString());
        }
        //preprend the line with "ret = ..."
        std::string toInsert("    ret = ");
        exprCpy.insert(0, toInsert);
    } else {
        exprCpy.insert(0, "    ");
        std::size_t foundNewLine = exprCpy.find("\n");
        while (foundNewLine != std::string::npos) {
            exprCpy.insert(foundNewLine + 1, "    ");
            foundNewLine = exprCpy.find("\n", foundNewLine + 1);
        }
    }

    KnobHolderPtr holder = getHolder();
    if (!holder) {
        throw std::runtime_error(tr("This parameter cannot have an expression").toStdString());
    }

    NodePtr node;
    EffectInstancePtr effect = toEffectInstance(holder);
    KnobTableItemPtr tableItem = toKnobTableItem(holder);
    if (effect) {
        node = effect->getNode();
    } else if (tableItem) {
        KnobItemsTablePtr model = tableItem->getModel();
        if (model) {
            node = model->getNode();
        }
    }
    if (!node) {
        throw std::runtime_error(tr("Only parameters of a Node can have an expression").toStdString());
    }
    std::string appID = holder->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string exprFuncPrefix = nodeFullName + "." + getName() + ".";

    // make-up the expression function
    std::string exprFuncName;
    {
        std::stringstream tmpSs;
        tmpSs << "expression" << dimension << "_" << view;
        exprFuncName = tmpSs.str();
    }

    exprCpy.append("\n    return ret\n");


    std::stringstream ss;
    ss << "def "  << exprFuncName << "(frame, view):\n";

    // First define the attributes that may be used by the expression
    ss << _imp->getReachablePythonAttributesForExpression(true, dimension, view);


    std::string script = ss.str();

    // Append the user expression
    script.append(exprCpy);

    // Set the expression function as an attribute of the knob itself
    script.append(exprFuncPrefix + exprFuncName + " = " + exprFuncName);

    // Try to execute the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    // with the error.
    std::string error;
    std::string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    {
        EXPR_RECURSION_LEVEL();

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) {
            throw std::runtime_error(error);
        }

        std::string viewName;
        if (getHolder() && getHolder()->getApp()) {
            viewName = getHolder()->getApp()->getProject()->getViewName(view);
        }
        if (viewName.empty()) {
            viewName = "Main";
        }

        std::stringstream ss;
        ss << funcExecScript << '(' << getCurrentTime_TLS() << ", \"" <<  viewName << "\")\n";
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &error, 0) ) {
            throw std::runtime_error(error);
        }

        PyObject *ret = PyObject_GetAttrString(NATRON_PYTHON_NAMESPACE::getMainModule(), "ret"); //get our ret variable created above

        if ( !ret || PyErr_Occurred() ) {
#ifdef DEBUG
            PyErr_Print();
#endif
            throw std::runtime_error("return value must be assigned to the \"ret\" variable");
        }


        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(this);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(this);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(this);
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(this);
        if (isDouble) {
            double r = isDouble->pyObjectToType<double>(ret, view);
            *resultAsString = QString::number(r).toStdString();
        } else if (isInt) {
            int r = isInt->pyObjectToType<int>(ret, view);
            *resultAsString = QString::number(r).toStdString();
        } else if (isBool) {
            bool r = isBool->pyObjectToType<bool>(ret, view);
            *resultAsString = r ? "True" : "False";
        } else {
            assert(isString);
            *resultAsString = isString->pyObjectToType<std::string>(ret, view);
        }
    }


    return funcExecScript;
} // KnobHelper::validateExpression

NATRON_NAMESPACE_ANONYMOUS_ENTER
struct ExprToReApply {
    ViewIdx view;
    DimIdx dimension;
    std::string expr;
    bool hasRet;
};
NATRON_NAMESPACE_ANONYMOUS_EXIT

bool
KnobHelper::checkInvalidExpressions()
{
    int ndims = getNDimensions();


    std::vector<ExprToReApply> exprToReapply;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < ndims; ++i) {
            for (ExprPerViewMap::const_iterator it = _imp->expressions[i].begin(); it != _imp->expressions[i].end(); ++it) {
                if (!it->second.exprInvalid.empty()) {
                    exprToReapply.resize(exprToReapply.size() + 1);
                    ExprToReApply& data = exprToReapply.back();
                    data.view = it->first;
                    data.dimension = DimIdx(i);
                    data.expr = it->second.originalExpression;
                    data.hasRet = it->second.hasRet;
                }
            }
        }
    }
    bool isInvalid = false;
    for (std::size_t i = 0; i < exprToReapply.size(); ++i) {
        setExpressionInternal(exprToReapply[i].dimension, exprToReapply[i].view, exprToReapply[i].expr, exprToReapply[i].hasRet, false /*throwOnFailure*/);
        std::string err;
        if ( !isExpressionValid(exprToReapply[i].dimension, exprToReapply[i].view, &err) ) {
            isInvalid = true;
        }
    }
    return !isInvalid;
}

bool
KnobHelper::isExpressionValid(DimIdx dimension,
                              ViewIdx view,
                              std::string* error) const
{
    if ( ( dimension >= (int)_imp->expressions.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::isExpressionValid(): Dimension out of range");
    }

    ViewIdx view_i = getViewIdxFromGetSpec(view);
    {
        QMutexLocker k(&_imp->expressionMutex);
        if (error) {
            ExprPerViewMap::const_iterator foundView = _imp->expressions[dimension].find(view_i);
            if (foundView != _imp->expressions[dimension].end()) {
                *error = foundView->second.exprInvalid;
                return error->empty();
            }
        }
    }
    return true;
}

void
KnobHelper::setExpressionInvalidInternal(DimIdx dimension, ViewIdx view, bool valid, const std::string& error)
{
    bool wasValid;
    {
        QMutexLocker k(&_imp->expressionMutex);
        ExprPerViewMap::iterator foundView = _imp->expressions[dimension].find(view);
        if (foundView == _imp->expressions[dimension].end()) {
            return;
        }
        wasValid = foundView->second.exprInvalid.empty();
        foundView->second.exprInvalid = error;
    }

    if (wasValid && !valid) {
        getHolder()->getApp()->addInvalidExpressionKnob( boost::const_pointer_cast<KnobI>( shared_from_this() ) );
        _signalSlotHandler->s_expressionChanged(dimension, view);
    } else if (!wasValid && valid) {
        bool haveOtherExprInvalid = false;
        {
            int ndims = getNDimensions();
            std::list<ViewIdx> views = getViewsList();
            QMutexLocker k(&_imp->expressionMutex);
            for (int i = 0; i < ndims; ++i) {
                if (i != dimension) {
                    for (ExprPerViewMap::const_iterator it = _imp->expressions[i].begin(); it != _imp->expressions[i].end(); ++it) {
                        if (it->first != view) {
                            if ( !it->second.exprInvalid.empty() ) {
                                haveOtherExprInvalid = true;
                                break;
                            }
                        }
                    }
                    if (haveOtherExprInvalid) {
                        break;
                    }
                }
            }
        }
        if (!haveOtherExprInvalid) {
            getHolder()->getApp()->removeInvalidExpressionKnob( shared_from_this() );
        }
        _signalSlotHandler->s_expressionChanged(dimension, view);

    }

}

void
KnobHelper::setExpressionInvalid(DimSpec dimension,
                                 ViewSetSpec view,
                                 bool valid,
                                 const std::string& error)
{
    if (!getHolder() || !getHolder()->getApp()) {
        return;
    }
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setExpressionInvalidInternal(DimIdx(i), *it, valid, error);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                setExpressionInvalidInternal(DimIdx(i), view_i, valid, error);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setExpressionInvalid(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setExpressionInvalidInternal(DimIdx(dimension), *it, valid, error);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            setExpressionInvalidInternal(DimIdx(dimension), view_i, valid, error);
        }
    }
} // setExpressionInvalid


void
KnobHelper::setExpressionInternal(DimIdx dimension,
                                  ViewIdx view,
                                  const std::string& expression,
                                  bool hasRetVariable,
                                  bool failIfInvalid)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    assert( dimension >= 0 && dimension < getNDimensions() );

    PythonGILLocker pgl;

    ///Clear previous expr
    clearExpression(dimension, ViewSetSpec(view));

    if ( expression.empty() ) {
        return;
    }

    std::string exprResult;
    std::string exprCpy;
    std::string exprInvalid;
    try {
        exprCpy = validateExpression(expression, dimension, view, hasRetVariable, &exprResult);
    } catch (const std::exception &e) {
        exprInvalid = e.what();
        exprCpy = expression;
        if (failIfInvalid) {
            throw std::invalid_argument(exprInvalid);
        }
    }

    // Set internal fields

    {
        QMutexLocker k(&_imp->expressionMutex);
        Expr& expr = _imp->expressions[dimension][view];
        expr.hasRet = hasRetVariable;
        expr.expression = exprCpy;
        expr.originalExpression = expression;
        expr.exprInvalid = exprInvalid;
    }

    KnobHolderPtr holder = getHolder();
    if (holder) {
        // Parse listeners of the expression, to keep track of dependencies to indicate them to the user.

        if ( exprInvalid.empty() ) {
            EXPR_RECURSION_LEVEL();
            _imp->parseListenersFromExpression(dimension, view);
        } else {
            AppInstancePtr app = holder->getApp();
            if (app) {
                app->addInvalidExpressionKnob( shared_from_this() );
            }
        }
    }


    // Notify the expr. has changed
    expressionChanged(dimension, view);
} // setExpressionInternal

void
KnobHelper::setExpressionCommon(DimSpec dimension,
                                ViewSetSpec view,
                                const std::string& expression,
                                bool hasRetVariable,
                                bool failIfInvalid)
{
    // setExpressionInternal may call evaluateValueChange if it calls clearExpression, hence bracket changes
    beginChanges();

    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setExpressionInternal(DimIdx(i), *it, expression, hasRetVariable, failIfInvalid);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                setExpressionInternal(DimIdx(i), view_i, expression, hasRetVariable, failIfInvalid);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setExpressionCommon(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setExpressionInternal(DimIdx(dimension), *it, expression, hasRetVariable, failIfInvalid);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            setExpressionInternal(DimIdx(dimension), view_i, expression, hasRetVariable, failIfInvalid);
        }
    }

    evaluateValueChange(dimension, getHolder()->getCurrentTime_TLS(), view, eValueChangedReasonUserEdited);
    endChanges();
} // setExpressionCommon

void
KnobHelper::replaceNodeNameInExpressionInternal(DimIdx dimension,
                                                ViewIdx view,
                                                const std::string& oldName,
                                                const std::string& newName)
{

    EffectInstancePtr isEffect = toEffectInstance(getHolder());
    if (!isEffect) {
        return;
    }

    std::string hasExpr = getExpression(dimension, view);
    if ( hasExpr.empty() ) {
        return;
    }
    bool hasRetVar = isExpressionUsingRetVariable(view, dimension);
    try {
        //Change in expressions the script-name
        QString estr = QString::fromUtf8( hasExpr.c_str() );
        estr.replace( QString::fromUtf8( oldName.c_str() ), QString::fromUtf8( newName.c_str() ) );
        hasExpr = estr.toStdString();
        setExpression(dimension, ViewSetSpec(view), hasExpr, hasRetVar, false);
    } catch (...) {
    }

} // replaceNodeNameInExpressionInternal

void
KnobHelper::replaceNodeNameInExpression(DimSpec dimension,
                                        ViewSetSpec view,
                                        const std::string& oldName,
                                        const std::string& newName)
{
    if (oldName == newName) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return;
    }
    holder->beginChanges();

    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    replaceNodeNameInExpressionInternal(DimIdx(i), *it, oldName, newName);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                replaceNodeNameInExpressionInternal(DimIdx(i), view_i, oldName, newName);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::replaceNodeNameInExpression(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                replaceNodeNameInExpressionInternal(DimIdx(dimension), *it, oldName, newName);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            replaceNodeNameInExpressionInternal(DimIdx(dimension), view_i, oldName, newName);
        }
    }

    holder->endChanges();

} // replaceNodeNameInExpression

bool
KnobHelper::isExpressionUsingRetVariable(ViewIdx view, DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_imp->expressions.size()) {
        throw std::invalid_argument("KnobHelper::isExpressionUsingRetVariable(): Dimension out of range");
    }
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker k(&_imp->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->expressions[dimension].find(view_i);
    if (foundView == _imp->expressions[dimension].end()) {
        return false;
    }
    return foundView->second.hasRet;
}

bool
KnobHelper::getExpressionDependencies(DimIdx dimension,
                                      ViewIdx view,
                                      KnobDimViewKeySet& dependencies) const
{
    if (dimension < 0 || dimension >= (int)_imp->expressions.size()) {
        throw std::invalid_argument("KnobHelper::getExpressionDependencies(): Dimension out of range");
    }
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker k(&_imp->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->expressions[dimension].find(view_i);
    if (foundView == _imp->expressions[dimension].end() || foundView->second.expression.empty()) {
        return false;
    }
    dependencies = foundView->second.dependencies;
    return true;
}

bool
KnobHelper::clearExpressionInternal(DimIdx dimension, ViewIdx view)
{
    PythonGILLocker pgl;
    bool hadExpression = false;
    KnobDimViewKeySet dependencies;
    {
        QMutexLocker k(&_imp->expressionMutex);
        ExprPerViewMap::iterator foundView = _imp->expressions[dimension].find(view);
        if (foundView != _imp->expressions[dimension].end()) {
            hadExpression = !foundView->second.originalExpression.empty();
            foundView->second.expression.clear();
            foundView->second.originalExpression.clear();
            foundView->second.exprInvalid.clear();

            dependencies = foundView->second.dependencies;
            foundView->second.dependencies.clear();
        }
    }
    KnobIPtr thisShared = shared_from_this();
    {

        // Notify all dependencies of the expression that they no longer listen to this knob
        KnobDimViewKey listenerToRemoveKey(thisShared, dimension, view);
        for (KnobDimViewKeySet::iterator it = dependencies.begin();
             it != dependencies.end(); ++it) {

            KnobIPtr otherKnob = it->knob.lock();
            KnobHelper* other = dynamic_cast<KnobHelper*>( otherKnob.get() );
            if (!other) {
                continue;
            }

            {
                QMutexLocker otherMastersLocker(&other->_imp->expressionMutex);

                KnobDimViewKeySet& otherListeners = other->_imp->expressions[it->dimension][it->view].listeners;
                KnobDimViewKeySet::iterator foundListener = otherListeners.find(listenerToRemoveKey);
                if (foundListener != otherListeners.end()) {
                    otherListeners.erase(foundListener);
                }
            }
        }
    }

    if (hadExpression) {
        expressionChanged(dimension, view);
    }
    return hadExpression;
} // clearExpressionInternal

void
KnobHelper::clearExpression(DimSpec dimension,
                            ViewSetSpec view)
{

    bool didSomething = false;
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    didSomething |= clearExpressionInternal(DimIdx(i), *it);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                didSomething |= clearExpressionInternal(DimIdx(i), view_i);
            }
        }
    } else {
        if ( ( dimension >= (int)_imp->expressions.size() ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::clearExpression(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                didSomething |= clearExpressionInternal(DimIdx(dimension), *it);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            didSomething |= clearExpressionInternal(DimIdx(dimension), view_i);
        }
    }


    if (didSomething) {
        evaluateValueChange(dimension, getHolder()->getTimelineCurrentTime(), view, eValueChangedReasonUserEdited);
    }

} // KnobHelper::clearExpression

void
KnobHelper::expressionChanged(DimIdx dimension, ViewIdx view)
{ 
    _signalSlotHandler->s_expressionChanged(dimension, view);

    computeHasModifications();
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

static bool
catchErrors(PyObject* mainModule,
            std::string* error)
{
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        ///Gui session, do stdout, stderr redirection
        if ( PyObject_HasAttrString(mainModule, "catchErr") ) {
            PyObject* errCatcher = PyObject_GetAttrString(mainModule, "catchErr"); //get our catchOutErr created above, new ref
            PyObject *errorObj = 0;
            if (errCatcher) {
                errorObj = PyObject_GetAttrString(errCatcher, "value"); //get the  stderr from our catchErr object, new ref
                assert(errorObj);
                *error = NATRON_PYTHON_NAMESPACE::PyStringToStdString(errorObj);
                PyObject* unicode = PyUnicode_FromString("");
                PyObject_SetAttrString(errCatcher, "value", unicode);
                Py_DECREF(errorObj);
                Py_DECREF(errCatcher);
            }
        }

        return false;
    }

    return true;
} // catchErrors

NATRON_NAMESPACE_ANONYMOUS_EXIT

bool
KnobHelper::executeExpression(TimeValue time,
                              ViewIdx view,
                              DimIdx dimension,
                              PyObject** ret,
                              std::string* error) const
{
    if (dimension < 0 || dimension >= (int)_imp->expressions.size()) {
        throw std::invalid_argument("KnobHelper::executeExpression(): Dimension out of range");
    }

    std::string expr;
    {
        QMutexLocker k(&_imp->expressionMutex);
        ExprPerViewMap::const_iterator foundView = _imp->expressions[dimension].find(view);
        if (foundView == _imp->expressions[dimension].end() || foundView->second.expression.empty()) {
            return false;
        }
        expr = foundView->second.expression;
    }

    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);
    std::stringstream ss;

    std::string viewName;
    if (getHolder() && getHolder()->getApp()) {
        viewName = getHolder()->getApp()->getProject()->getViewName(view);
    }
    if (viewName.empty()) {
        viewName = "Main";
    }

    ss << expr << '(' << time << ", \"" << viewName << "\")\n";
    std::string script = ss.str();
    PyObject* v = PyRun_String(script.c_str(), Py_file_input, globalDict, 0);
    Py_XDECREF(v);

    *ret = 0;

    if ( !catchErrors(mainModule, error) ) {
        return false;
    }
    *ret = PyObject_GetAttrString(mainModule, "ret"); //get our ret variable created above
    if (!*ret) {
        *error = "Missing ret variable";
        
        return false;
    }
    if ( !catchErrors(mainModule, error) ) {
        return false;
    }
    
    return true;
} // executeExpression

std::string
KnobHelper::getExpression(DimIdx dimension, ViewIdx view) const
{
    if (dimension < 0 || dimension >= (int)_imp->expressions.size()) {
        throw std::invalid_argument("Knob::getExpression: Dimension out of range");
    }
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker k(&_imp->expressionMutex);
    ExprPerViewMap::const_iterator foundView = _imp->expressions[dimension].find(view_i);
    if (foundView == _imp->expressions[dimension].end() || foundView->second.expression.empty()) {
        return std::string();
    }
    return foundView->second.originalExpression;
}



NATRON_NAMESPACE_EXIT
