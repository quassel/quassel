#include <QFile>
#include <QDebug>
#include <QtXml>
#include <QxtHtmlTemplate>
#include <QStringList>
#include <QPair>
#include <QHash>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QProcess>


struct Module;
struct Class
{
    QString name;
    QString ref;

    QString desc;

    Module * module;

};
struct Module
{
    QString name;
    QString ref;

    QString desc;


    QList<Class *> classes;
};


bool sortClassBynameLessThen(const Class *s1, const Class *s2)
{
    return s1->name < s2->name;
}
bool sortModuleBynameLessThen(const Module *s1, const Module *s2)
{
    return s1->name < s2->name;
}




///information collected from the xml files
QList<Class *> classes;
QList<Class *> publiclasses;
QList<Module *> modules;

///settings
QString outputDir;
QString templateDir;
QString xmlDir;




Class * findClassByRef(QString ref)
{
    foreach(Class * c,classes)
    {
        if (c->ref==ref)
            return c;
    }
    qFatal("ref %s invalid",qPrintable(ref));
    return 0;
}







QString refToLink( QString ref)
{
    QStringList e=ref.split("_");

    QString object=e.at(0);
    QString sub;
    if(e.count()>1)
        sub=e.at(1);



    ///FIXME that's a dirty hack. Might not actualy be sane
    ///TODO external reference resolving
    if (!object.contains("Qxt"))
    {
        object="http://doc.trolltech.com/latest/"+object;

        if (sub.startsWith("1"))
            sub=sub.mid(1);
    }


    if(sub.size())
        return object+".html#"+sub;
    else
        return object+".html";
}







QString descRTF(QDomElement element)
{
    ///TODO parse the rest




    QString text;


    for(QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
    {
        if (n.isElement ())
        {
            QDomElement e= n.toElement();
            if(e.tagName ()=="para")
            {
                text += "<p>"+descRTF(e)+"</p>";
            }
            else if(e.tagName ()=="programlisting")
            {
                text += "<div class=\"code\">"+descRTF(e)+"</div>";
            }
            else if(e.tagName ()=="codeline")
            {
                text += descRTF(e)+"<br/>\r\n";
            }
            else if(e.tagName ()=="highlight")
            {
                text += "<span class=\"highlight_"+e.attribute("class")+"\"  >"+descRTF(e)+"</span>";
            }
            else if(e.tagName ()=="ref")
            {
                ///ignore namespaces, we don't have them
                if(e.attribute("refid").startsWith("namespace"))
                    text +=descRTF(e);
                else
                    text += "<a class=\"reflink\" href=\""+refToLink(e.attribute("refid"))+"\">"+descRTF(e)+"</a>";
            }
            else if(e.tagName ()=="image")
            {
                QString s=descRTF(e);
                text += "<table class=\"descimg\" ><tr><td><img alt=\""+s+"\" src=\""+e.attribute("name")+"\"></td></tr>";
                text += "<tr><td><sup>"+s+"</sup></td></tr></table>";
            }
            else if(e.tagName ()=="linebreak")
            {
                text += "<br/>\r\n";
            }
            else 
            {
                 text += e.text().replace("<","&lt;").replace(">","&gt;")+" ";
            }
        }
        else if (n.isText ()) 
        {
            text += n.toText().data().replace("<","&lt;").replace(">","&gt;");
        }
    }
    return text;
}






///fill classes and modules globals
void parseIndex(QString location)
{


    QDomDocument doc("doc");
    QFile file(location+"/index.xml");
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open file");
    QString  errorMsg;
    int errorLine=0;
    int errorColumn=0;

    if (!doc.setContent(&file,&errorMsg,&errorLine,&errorColumn)) 
    {
        qCritical("%s:%i:%i %s",qPrintable(location+"/index.xml"),errorLine,errorColumn,qPrintable(errorMsg));
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    if(docElem.tagName ()!="doxygenindex")
        qFatal("unexpected top node in %s",qPrintable(location+"/index.xml"));


    QDomElement e = docElem.firstChildElement("compound");
    while(!e.isNull()) 
    {
        if (e.attribute("kind")=="class")
        {
            Class * cl=new Class;
            cl->module=0;
            cl->name=e.firstChildElement("name").text();
            cl->ref=e.attribute("refid");
            classes.append(cl);
        }
        else if (e.attribute("kind")=="group")
        {
            Module * cl=new Module;
            cl->name=e.firstChildElement("name").text();
            cl->ref=e.attribute("refid");
            modules.append(cl);
        }
        else
        {
            qWarning("no way to document %s",qPrintable(e.attribute("kind")));
        }
    e = e.nextSiblingElement("compound");
    }

}



void parseModule(QString location,Module *m)
{
    QDomDocument doc("doc");
    QString filename=location+"/"+m->ref+".xml";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open file %s",qPrintable(filename));
    QString  errorMsg;
    int errorLine=0;
    int errorColumn=0;

    if (!doc.setContent(&file,&errorMsg,&errorLine,&errorColumn)) 
    {
        qCritical("%s:%i:%i %s",qPrintable(filename),errorLine,errorColumn,qPrintable(errorMsg));
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    QDomElement def  = docElem.firstChildElement("compounddef");
    if(def.attribute("id")!=m->ref)
        qFatal("combound def %s not expected in %s",qPrintable(def.attribute("id")),qPrintable(filename));



    m->desc=descRTF(def.firstChildElement("detaileddescription"));



    QDomElement e = def.firstChildElement("innerclass");
    while(!e.isNull()) 
    {
        if (e.attribute("prot")=="public")
        {
            Class * cll=findClassByRef(e.attribute("refid"));
            m->classes.append(cll);
            cll->module=m;
        }
        else
        {
            qWarning("non public member in %s",qPrintable(m->ref));
        }
    e = e.nextSiblingElement("innerclass");
    }

}


QString printPublicClasses()
{

    QxtHtmlTemplate t;
    if(!t.open(templateDir+"/classes.html"))qFatal("cannot open template");
    QxtHtmlTemplate t_i;
    if(!t_i.open(templateDir+"/classes-unroll.html"))qFatal("cannot open template");


    uint trs=classes.count()/4;
    QHash<uint,QString> rowstring;
    uint cr=1;

    QString lastChar=" ";
    foreach(Class * cl,publiclasses)
    {
        if (cl->name.count()<3)qFatal("bad class name %s",qPrintable(cl->name));


        if(cl->name.at(3)!=lastChar.at(0))
        {
            lastChar=cl->name.at(3);
            rowstring[cr]+="<th>"+lastChar+"</th>";
            cr++;
            if(cr>trs)
                cr=1;
        }

        t_i.clear();
        t_i["name"]=cl->name;
        t_i["link"]=refToLink(cl->ref);

        rowstring[cr]+=t_i.render();
        cr++;
        if(cr>trs)
            cr=1;
    }

    foreach(QString rowstr, rowstring.values())
    {
        t["unroll"]+="<tr>\r\n"+rowstr+"</tr>\r\n\r\n";
    }
    return t.render();;
}






QString printModules()
{
    QxtHtmlTemplate t;
    if(!t.open(templateDir+"/modules.html"))qFatal("cannot open template");
    QxtHtmlTemplate t_i;
    if(!t_i.open(templateDir+"/modules-unroll.html"))qFatal("cannot open template");


    int i=0;


    foreach(Module * cl,modules)
    {
        i++;
        t_i.clear();
        t_i["iseven"]=QString::number(i%2);
        t_i["name"]=cl->name;
        t_i["link"]=cl->ref+".html";
        t_i["desc"]=cl->desc;
        t["unroll"]+=t_i.render();
    }
    return t.render();;
}








QString printClass(QString location,Class * cl)
{
    QDomDocument doc("doc");
    QString filename=location+"/"+cl->ref+".xml";

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open file %s",qPrintable(filename));

    QString  errorMsg;
    int errorLine=0;
    int errorColumn=0;

    if (!doc.setContent(&file,&errorMsg,&errorLine,&errorColumn)) 
    {
        qCritical("%s:%i:%i %s",qPrintable(filename),errorLine,errorColumn,qPrintable(errorMsg));
        return QString("%1:%2:%3 %4").arg(filename).arg(errorLine)
            .arg(errorColumn).arg(errorMsg);
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    if(docElem.tagName ()!="doxygen")
        qFatal("unexpected top node in %s",qPrintable(filename));
    QDomElement def  = docElem.firstChildElement("compounddef");



    QxtHtmlTemplate t;
    if(!t.open(templateDir+"/class.html"))qFatal("cannot open template");




    ///name
    t["class_name"]=def.firstChildElement("compoundname").text();
    t["ref"]=def.attribute("id");

    if(cl->module)
    {
        t["module_name"]=cl->module->name;
        t["module_link"]=cl->module->ref+".html";
    }
    else
        qWarning("class %s has no module defined",qPrintable(cl->name));




    ///description
    cl->desc=def.firstChildElement("briefdescription").text();
    t["desc_short"]=cl->desc;
    t["desc_detailed"]=descRTF(def.firstChildElement("detaileddescription"));



    ///inherits
    t["inherits"]="";
    QDomElement basecompoundref =def.firstChildElement("basecompoundref");
    if(!basecompoundref.isNull())
    {
        QxtHtmlTemplate t_i;
        if(!t_i.open(templateDir+"/class-unroll-inherits.html"))qFatal("cannot open template");
        t_i["name"]=basecompoundref.text();
        t_i["link"]=refToLink(basecompoundref.attribute("refid"));
        t["inherits"]=t_i.render();
    }



    ///sections
    t["sections"]="";
    QxtHtmlTemplate t_section,t_members_unroll,t_impl;
    if(!t_section.open(templateDir+"/class-section.html"))qFatal("cannot open template");
    if(!t_members_unroll.open(templateDir+"/class-section-unroll.html"))qFatal("cannot open template");
    if(!t_impl.open(templateDir+"/class-impl.html"))qFatal("cannot open template");

    QDomElement sectiondef=def.firstChildElement("sectiondef");
    while(!sectiondef.isNull()) 
    {
        if(sectiondef.attribute("kind").startsWith("private"))///skip private stuff
        {
            sectiondef = sectiondef.nextSiblingElement("sectiondef");
            continue;
        }


        t_section.clear();

        t_section["kind"]=sectiondef.attribute("kind");
        t_section["desc"]=sectiondef.attribute("kind"); ///TODO: map kind to desc




        qDebug()<<"parsing section "<<t_section["kind"];

        QDomElement member=sectiondef.firstChildElement("memberdef");
        while(!member.isNull()) 
        {
        qDebug()<<"parsing member "<<member.firstChildElement("name").text();

            t_members_unroll.clear();
            t_members_unroll["name"]=member.firstChildElement("name").text();
            t_members_unroll["signature"]=member.firstChildElement("argsstring").text();
            t_members_unroll["type"]=member.firstChildElement("type").text();
            t_members_unroll["link"]=refToLink(member.attribute("id"));

            t_section["list"]+=t_members_unroll.render();



            ///Member Function Documentation (impl) 
            t_impl.clear();

            QStringList lii=member.attribute("id").split("_");
            if(lii.count()>1)
                t_impl["ref"]=lii.at(1);
            else
                t_impl["ref"]=lii.at(0);

            t_impl["name"]=member.firstChildElement("name").text();
            t_impl["signature"]=member.firstChildElement("argsstring").text();
            t_impl["type"]=member.firstChildElement("type").text();
            t_impl["desc"]=descRTF(member.firstChildElement("detaileddescription"));


            t["impl"]+=t_impl.render();

            member = member.nextSiblingElement("memberdef");
        }


        t["sections"]+=t_section.render();

        sectiondef = sectiondef.nextSiblingElement("sectiondef");
    }


    return t.render();
}







QString printModule(Module * m)
{
    QxtHtmlTemplate t;
    if(!t.open(templateDir+"/module.html"))qFatal("cannot open template");
    QxtHtmlTemplate t_i;
    if(!t_i.open(templateDir+"/modules-unroll.html"))qFatal("cannot open template");

    t["name"]+=m->name;
    t["desc"]+=m->desc;

    int i=0;
    qSort(m->classes.begin(), m->classes.end(), sortClassBynameLessThen);
    foreach(Class * cl,m->classes)
    {
        i++;
        t_i.clear();
        t_i["iseven"]=QString::number(i%2);
        t_i["name"]=cl->name;
        t_i["link"]=cl->ref+".html";
        t_i["desc"]=cl->desc;
        t["unroll"]+=t_i.render();
    }
    return t.render();;
}





QString printListOfMembers(QString location,Class * cl)
{
    QDomDocument doc("doc");
    QFile file(location+"/"+cl->ref+".xml");
    if (!file.open(QIODevice::ReadOnly))
        qFatal("cannot open file %s",qPrintable(location+"/"+cl->ref+".xml"));
    QString  errorMsg;
    int errorLine=0;
    int errorColumn=0;

    if (!doc.setContent(&file,&errorMsg,&errorLine,&errorColumn)) 
    {
        qCritical("%s:%i:%i %s",qPrintable(location+"/"+cl->ref+".xml"),errorLine,errorColumn,qPrintable(errorMsg));
        return QString("%1:%2:%3 %4").arg(location+"/index.xml").arg(errorLine)
            .arg(errorColumn).arg(errorMsg);
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    if(docElem.tagName ()!="doxygen")
        qFatal("unexpected top node in %s",qPrintable(location+"/"+cl->ref+".xml"));
    QDomElement def  = docElem.firstChildElement("compounddef");



    QxtHtmlTemplate t;
    if(!t.open(templateDir+"/class-members.html"))qFatal("cannot open template");


    ///name
    t["class_name"]=def.firstChildElement("compoundname").text();
    t["ref"]=def.attribute("id");

    ///list
    QDomElement list =def.firstChildElement("listofallmembers");
    t["list"]="";

    QxtHtmlTemplate t_i;
    if(!t_i.open(templateDir+"/class-members-unroll.html"))
        qFatal("cannot open template");


    QDomElement member=list.firstChildElement("member");
    while(!member.isNull()) 
    {
        if(member.attribute("prot")=="private")///skip private members
        {
            member = member.nextSiblingElement("memberdef");
            continue;
        }



        t_i.clear();
        t_i["name"]=member.firstChildElement("name").text();
        t_i["link"]=refToLink(member.attribute("refid"));
        t["list"]+=t_i.render();
        member = member.nextSiblingElement("member");
    }

    return t.render();
}







void wrapToFile(QString filename,QString content)
{

    QxtHtmlTemplate site;
    if(!site.open(templateDir+"/site.html"))qFatal("cannot open template");
    site["content"]=content;

    QFile file(outputDir+"/"+filename);
    if (!file.open(QIODevice::WriteOnly))
        qFatal("cannot open output file %s",qPrintable(filename));

    file.write(site.render().toUtf8());

    file.close();
}






int main(int argc,char ** argv)
{

    QCoreApplication app(argc,argv);
    qDebug("[greetings]");


    QString settingsfile="Doqsyfile";


    if(app.arguments().count()>1)
    {
       settingsfile=app.arguments().at(1);
    }

    if(!QFileInfo(settingsfile).exists())
        qFatal("cannot open %s",qPrintable(settingsfile));

    if (!QDir::setCurrent (QFileInfo(settingsfile).absolutePath ()))
        qFatal("unable to change working directory to %s",qPrintable(QFileInfo(settingsfile).absolutePath ()));

    QSettings settings(settingsfile,QSettings::IniFormat);
    settings.beginGroup ("doqsy");
    outputDir=settings.value("output","doc").toString();
    templateDir=settings.value("templates","templates").toString();
    QString doxyexe=settings.value("doxygen","doxygen").toString();



    QDir().mkpath(outputDir);
    settings.endGroup();



    if(!QDir::temp().mkpath("doqsytmp"))
        qFatal("cannot make  temp dir");
    xmlDir=QDir::tempPath()+"doqsytmp";



    

    QString doxygeninput;

    settings.beginGroup ("doxygen");
    foreach(QString key,settings.allKeys())
    {
        doxygeninput+=(key+"="+settings.value(key).toString()+"\r\n");
    }
    settings.endGroup();

    doxygeninput+=  "XML_OUTPUT             = "+xmlDir+"\r\n"
                        "OUTPUT_DIRECTORY       = "+QDir::tempPath()+"\r\n"
                        "GENERATE_XML           = YES\r\n";



    qDebug("[running doxygen]");


    QProcess doxygenprocess;

    doxygenprocess.setProcessChannelMode(QProcess::ForwardedChannels);

    doxygenprocess.setWorkingDirectory (QDir().absolutePath ());

    doxygenprocess.start (doxyexe,QStringList()<<"-");

    if(!doxygenprocess.waitForStarted ())
        qFatal("doxygen failed to start");

    doxygenprocess.write(doxygeninput.toUtf8());

    doxygenprocess.closeWriteChannel();

    if(!doxygenprocess.waitForFinished (120000))
        qFatal("doxygen failed to finish within 2 minutes");

    if(doxygenprocess.exitCode ())
        qFatal("doxygen run unsecussfull");


    qDebug("[beginn parsing]");

    parseIndex(xmlDir);


    qSort(classes.begin(), classes.end(), sortClassBynameLessThen);
    qSort(modules.begin(), modules.end(), sortModuleBynameLessThen);

    foreach(Module *  m,modules)
    {
        qDebug()<<"parsing module"<<m->ref;
        parseModule(xmlDir,m);
        publiclasses+=m->classes;
    }

    wrapToFile("modules.html",printModules());

    qSort(publiclasses.begin(), publiclasses.end(), sortClassBynameLessThen);
    wrapToFile("classes.html",printPublicClasses());





    foreach(Class * c,classes)
    {
        qDebug()<<"parsing class "<<c->name;
        wrapToFile(c->ref+".html",printClass(xmlDir,c));
        wrapToFile(c->ref+"-members.html",printListOfMembers(xmlDir,c));
    }

    foreach(Module *  m,modules)
    {
        wrapToFile(m->ref+".html",printModule(m));
    }




    QxtHtmlTemplate t_i;
    if(!t_i.open(templateDir+"/index.html"))
        qFatal("cannot open template");
    
    wrapToFile("index.html",t_i.render());
    qDebug("[done]");
    return 0;
}

