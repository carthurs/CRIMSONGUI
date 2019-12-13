#pragma once

#include <QDoubleSpinBox>
#include <iostream>

class QDoubleSpinBoxG : public QDoubleSpinBox {
    Q_OBJECT
public:
    QDoubleSpinBoxG(QWidget *parent = 0) : QDoubleSpinBox(parent) {}
    ~QDoubleSpinBoxG() {}

    QString textFromValue(double value) const override
    {
        QString format = "%." + QString::number(decimals()) + "g";
        QString str;
        str.sprintf(format.toStdString().c_str(), value);
        if (qAbs(value) >= 1000.0) {
            str.remove(locale().groupSeparator());
        }
        return str;
    }

    QValidator::State validate(QString &text, int &pos) const override
    {
        QValidator::State state;
        validateAndInterpret(text, pos, state);
        return state;
    }

    QString stripped(const QString &t, int *pos) const
    {
        QString text = t;
        QString prefixtext = prefix();
        QString suffixtext = suffix();

        if (specialValueText().size() == 0 || text != specialValueText()) {
            int from = 0;
            int size = text.size();
            bool changed = false;
            if (prefixtext.size() && text.startsWith(prefixtext)) {
                from += prefixtext.size();
                size -= from;
                changed = true;
            }
            if (suffixtext.size() && text.endsWith(suffixtext)) {
                size -= suffixtext.size();
                changed = true;
            }
            if (changed)
                text = text.mid(from, size);
        }

        const int s = text.size();
        text = text.trimmed();
        if (pos)
            (*pos) -= (s - text.size());
        return text;
    }

    QVariant validateAndInterpret(QString &input, int &pos,
        QValidator::State &state) const
    {
        static QString cachedText;
        static QValidator::State cachedState;
        static QVariant cachedValue;
        if (cachedText == input && !input.isEmpty()) {
            state = cachedState;
            return cachedValue;
        }
        const double max = maximum();
        const double min = minimum();

        QString copy = stripped(input, &pos);
        int len = copy.size();
        double num = min;
        const bool plus = max >= 0;
        const bool minus = min <= 0;

        switch (len) {
        case 0:
            state = max != min ? QValidator::Intermediate : QValidator::Invalid;
            goto end;
        case 1:
            if (copy.at(0) == locale().decimalPoint()
                || (plus && copy.at(0) == QLatin1Char('+'))
                || (minus && copy.at(0) == QLatin1Char('-'))) {
                state = QValidator::Intermediate;
                goto end;
            }
            break;
        case 2:
            if (copy.at(1) == locale().decimalPoint()
                && ((plus && copy.at(0) == QLatin1Char('+')) || (minus && copy.at(0) == QLatin1Char('-')))) {
                state = QValidator::Intermediate;
                goto end;
            }
            break;
        default: break;
        }

        if (copy.at(0) == locale().groupSeparator()) {
            state = QValidator::Invalid;
            goto end;
        }
        else if (len > 1) {
            const int dec = copy.indexOf(locale().decimalPoint());
            if (dec != -1) {
                if (dec + 1 < copy.size() && copy.at(dec + 1) == locale().decimalPoint() && pos == dec + 1) {
                    copy.remove(dec + 1, 1); // typing a delimiter when you are on the delimiter
                } // should be treated as typing right arrow

                if (copy.size() - dec > decimals() + 1) {
                    state = QValidator::Invalid;
                    goto end;
                }
                for (int i = dec + 1; i < copy.size(); ++i) {
                    if (copy.at(i).isSpace() || copy.at(i) == locale().groupSeparator()) {
                        state = QValidator::Invalid;
                        goto end;
                    }
                }
            }
            else {
                const QChar &last = copy.at(len - 1);
                const QChar &secondLast = copy.at(len - 2);
                if ((last == locale().groupSeparator() || last.isSpace())
                    && (secondLast == locale().groupSeparator() || secondLast.isSpace())) {
                    state = QValidator::Invalid;
                    goto end;
                }
            }
            const QChar &last = copy.at(len - 1);
            const QChar &secondLast = copy.at(len - 2);
            if (last.isSpace() && (!locale().groupSeparator().isSpace() || secondLast.isSpace())) {
                state = QValidator::Invalid;
                goto end;
            }
            else if (last == locale().exponential() ||
                (secondLast == locale().exponential() && (last == QLatin1Char('+') || last == QLatin1Char('-')))) {
                state = QValidator::Intermediate;
                goto end;
            }
        }

    {
        bool ok = false;
        num = locale().toDouble(copy, &ok);

        if (!ok) {
            if (locale().groupSeparator().isPrint()) {
                if (max < 1000 && min > -1000 && copy.contains(locale().groupSeparator())) {
                    state = QValidator::Invalid;
                    goto end;
                }

                const int len = copy.size();
                for (int i = 0; i < len - 1; ++i) {
                    if (copy.at(i) == locale().groupSeparator() && copy.at(i + 1) == locale().groupSeparator()) {
                        state = QValidator::Invalid;
                        goto end;
                    }
                }

                QString copy2 = copy;
                copy2.remove(locale().groupSeparator());
                num = locale().toDouble(copy2, &ok);

                if (!ok) {
                    state = QValidator::Invalid;
                    goto end;
                }
            }
        }

        if (!ok) {
            state = QValidator::Invalid;
        }
        else if (num >= min && num <= max) {
            state = QValidator::Acceptable;
        }
        else if (max == min) { // when max and min is the same the only non-Invalid input is max (or min)
            state = QValidator::Invalid;
        }
        else {
            if ((num >= 0 && num > max) || (num < 0 && num < min)) {
                state = QValidator::Invalid;
            }
            else {
                state = QValidator::Intermediate;
            }
        }
    }

    end:
        if (state != QValidator::Acceptable) {
            num = max > 0 ? min : max;
        }

        input = prefix() + copy + suffix();
        cachedText = input;
        cachedState = state;
        cachedValue = QVariant(num);
        return QVariant(num);
    }
};


