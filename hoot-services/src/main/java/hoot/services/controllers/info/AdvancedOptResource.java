/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2015, 2016 DigitalGlobe (http://www.digitalglobe.com/)
 */
package hoot.services.controllers.info;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.Iterator;
import java.util.Map;

import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import hoot.services.HootProperties;
import hoot.services.utils.ResourceErrorHandler;


@Path("/advancedopts")
public class AdvancedOptResource {
    private static final Logger logger = LoggerFactory.getLogger(AdvancedOptResource.class);
    private static final String asciidocPath = HootProperties.getProperty("configAsciidocPath");
    private static final String templatePath = HootProperties.getProperty("advOptTemplate");
    private static final String homeFolder = HootProperties.getProperty("homeFolder");
    private static final String refOverridePath = HootProperties.getProperty("advOptRefOverride");
    private static final String horzOverridePath = HootProperties.getProperty("advOptHorizontalOverride");
    private static final String aveOverridePath = HootProperties.getProperty("advOptAverageOverride");

    private JSONObject doc;
    private JSONArray template;
    private JSONArray refTemplate;
    private JSONArray hrzTemplate;
    private JSONArray aveTemplate;
    private JSONObject refOverride;
    private JSONObject horzOverride;
    private JSONObject aveOverride;

    static {
        File file = new File(homeFolder + "/" + asciidocPath);
        if (!file.exists()) {
            throw new RuntimeException("Missing required file: " + file.getAbsolutePath());
        }
    }

    public AdvancedOptResource() {
    }

    @GET
    @Path("/getoptions")
    @Produces(MediaType.TEXT_PLAIN)
    public Response getOptions(@QueryParam("conftype") String confType, @QueryParam("force") String isForce) {
        JSONArray template = null;
        try {
            // Force option should only be used to update options list by administrator
            boolean doForce = false;
            if (isForce != null) {
                doForce = isForce.equalsIgnoreCase("true");
            }

            getOverrides(isForce);
            if ((doc == null) || doForce) {
                doc = new JSONObject();
                parseAsciidoc();
            }

            if (doc != null) {
                JSONParser par = new JSONParser();
                if (confType.equalsIgnoreCase("reference")) {
                    if ((refTemplate == null) || doForce) {
                        refTemplate = new JSONArray();
                        refTemplate.add(refOverride);
                    }
                    template = refTemplate;
                }
                else if (confType.equalsIgnoreCase("horizontal")) {
                    if ((hrzTemplate == null) || doForce) {
                        hrzTemplate = new JSONArray();
                        hrzTemplate.add(horzOverride);
                    }
                    template = hrzTemplate;
                }
                else if (confType.equalsIgnoreCase("average")) {
                    if ((aveTemplate == null) || doForce) {
                        aveTemplate = new JSONArray();
                        aveTemplate.add(aveOverride);
                    }
                    template = aveTemplate;
                }
                else {
                    if ((doc == null) || (this.template == null) || doForce) {
                        try (FileReader fr = new FileReader(homeFolder + "/" + templatePath)) {
                            this.template = (JSONArray)par.parse(fr);
                            generateRule(this.template, null);
                        }
                    }
                    template = this.template;
                }
            }
            else {
                throw new Exception("Failed to populate asciidoc information.");
            }
        }
        catch (Exception ex) {
            ResourceErrorHandler.handleError("Error getting advanced options: " + ex, Status.INTERNAL_SERVER_ERROR, logger);
        }

        assert (template != null);
        return Response.ok(template.toJSONString(), MediaType.APPLICATION_JSON).build();
    }

    private void getOverrides(String isForce) throws Exception {
        // Force option should only be used to update options list by administrator
        boolean doForce = false;
        if (isForce != null) {
            doForce = isForce.equalsIgnoreCase("true");
        }

        if ((horzOverride == null) || (refOverride == null) || doForce) {
            JSONParser par = new JSONParser();

            try (FileReader frRef = new FileReader(homeFolder + File.separator + refOverridePath)){
                refOverride = (JSONObject) par.parse(frRef);
            }

            try (FileReader frHrz = new FileReader(homeFolder + File.separator + horzOverridePath)){
                horzOverride = (JSONObject) par.parse(frHrz);
            }

            try (FileReader frAve = new FileReader(homeFolder + File.separator + aveOverridePath)) {
                aveOverride = (JSONObject) par.parse(frAve);
            }
        }
    }

    private String getDepKeyVal(String sDefVal) {
        try {
            int iStart = sDefVal.indexOf('{') + 1;
            int iEnd = sDefVal.indexOf('}');
            String depKey = sDefVal.substring(iStart, iEnd).trim();

            if ((depKey != null) && (!depKey.isEmpty())) {
                // now find the dep value
                JSONObject oDep = (JSONObject) doc.get(depKey);
                return oDep.get("Default Value").toString();
            }
        }
        catch (Exception ignored) {
            logger.debug(ignored.getMessage());
        }
        return "";
    }

    private void generateRule(JSONArray a, JSONObject p) throws Exception {
        // for each options in template apply the value
        for (Object o : a) {
            if (o instanceof JSONObject) {
                JSONObject curOpt = (JSONObject) o;
                // first check to see if there is key then apply the asciidoc
                // val
                Object oKey = curOpt.get("hoot_key");
                if (oKey != null) {
                    String sKey = oKey.toString();
                    Object oDocKey = doc.get(sKey);
                    if (oDocKey != null) {
                        JSONObject jDocKey = (JSONObject) oDocKey;

                        Object oAttrib = jDocKey.get("Data Type");
                        if (oAttrib != null) {
                            String sDataType = oAttrib.toString().trim();
                            // We can not determine list vs multilist from
                            // asciidoc so we will skip for the data type
                            if (!sDataType.equals("list") && (!sDataType.isEmpty())) {
                                curOpt.put("elem_type", sDataType);
                                if (sDataType.equalsIgnoreCase("bool")) {
                                    // bool type is checkbox
                                    curOpt.put("elem_type", "checkbox");
                                }
                            }
                        }

                        oAttrib = jDocKey.get("Default Value");
                        if (oAttrib != null) {
                            String sDefVal = oAttrib.toString().trim();
                            if (!sDefVal.isEmpty()) {
                                // It is poting to other val
                                if (sDefVal.startsWith("$")) {
                                    String depDefVal = getDepKeyVal(sDefVal);
                                    curOpt.put("defaultvalue", depDefVal);
                                }
                                else {
                                    curOpt.put("defaultvalue", sDefVal);
                                }
                            }
                        }

                        oAttrib = jDocKey.get("Description");
                        if (oAttrib != null) {
                            String sDesc = oAttrib.toString().trim();
                            if (!sDesc.isEmpty()) {
                                curOpt.put("description", sDesc);
                            }
                        }

                        // handle override
                        oAttrib = curOpt.get("override");
                        if (oAttrib != null) {
                            JSONObject override = (JSONObject) oAttrib;
                            Iterator it = override.entrySet().iterator();

                            while (it.hasNext()) {
                                Map.Entry pair = (Map.Entry) it.next();
                                curOpt.put(pair.getKey(), pair.getValue());
                            }
                            // remove override element
                            curOpt.remove("override");
                        }
                    }
                }
                else {
                    // Second check for hoot_val and if there is one then apply
                    // descripton from asciidoc.  This can be checked from parent object
                    Object oVal = curOpt.get("hoot_val");
                    if (oVal != null) {
                        String sVal = oVal.toString();
                        if (p != null) {
                            // parent always have to have hoot_key for hoot_val
                            Object pKey = p.get("hoot_key");
                            if (pKey != null) {
                                // try to get default list from parent
                                String sPKey = pKey.toString();
                                Object oDocPKey = doc.get(sPKey);
                                if (oDocPKey != null) {
                                    JSONObject jDocPKey = (JSONObject) oDocPKey;
                                    Object oDocDefList = jDocPKey.get("Default List");

                                    if (oDocDefList != null) {
                                        JSONObject defList = (JSONObject) oDocDefList;
                                        Iterator it = defList.entrySet().iterator();
                                        boolean hasNext = true;
                                        while (hasNext) {
                                            Map.Entry pair = (Map.Entry) it.next();
                                            hasNext = it.hasNext();
                                            if (pair.getKey().toString().equalsIgnoreCase(sVal)) {
                                                String sDefListDesc = pair.getValue().toString().trim();
                                                if (!sDefListDesc.isEmpty()) {
                                                    curOpt.put("description", sDefListDesc);
                                                    curOpt.put("defaultvalue", "true");
                                                }
                                                hasNext = false;
                                            }
                                        }
                                    }

                                    // If the parent is boolean then try to get default value
                                    Object oDocDataType = jDocPKey.get("Data Type");
                                    if ((oDocDataType != null) && oDocDataType.toString().equalsIgnoreCase("bool")) {
                                        Object oDocDefVal = jDocPKey.get("Default Value");
                                        if (oDocDefVal != null) {
                                            if (oDocDefVal.toString().equalsIgnoreCase("true")) {
                                                if (sVal.equalsIgnoreCase("true")) {
                                                    curOpt.put("isDefault", "true");
                                                }

                                                if (sVal.equalsIgnoreCase("false")) {
                                                    curOpt.put("isDefault", "false");
                                                }
                                            }

                                            if (oDocDefVal.toString().equalsIgnoreCase("false")) {
                                                if (sVal.equalsIgnoreCase("true")) {
                                                    curOpt.put("isDefault", "false");
                                                }

                                                if (sVal.equalsIgnoreCase("false")) {
                                                    curOpt.put("isDefault", "true");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // handle override
                        Object oAttrib = curOpt.get("override");
                        if (oAttrib != null) {
                            JSONObject override = (JSONObject) oAttrib;

                            Iterator it = override.entrySet().iterator();
                            while (it.hasNext()) {
                                Map.Entry pair = (Map.Entry) it.next();
                                curOpt.put(pair.getKey(), pair.getValue());
                            }
                            // remove override element
                            curOpt.remove("override");
                        }
                    }
                }

                // now check for members and if one exist then recurse
                Object oValMembers = curOpt.get("members");
                if ((oValMembers != null) && (oValMembers instanceof JSONArray)) {
                    generateRule((JSONArray) oValMembers, curOpt);
                }
            }
        }

        // do recursion to visit each object and try to update value
    }

    // If line starts with "* " then attribute field
    // "Data Type:"
    // "Default Value:"
    // If line starts with "** " then list item field
    private String parseLine(String line, String curOptName) throws Exception {
        String optName = curOptName;
        // If line starts with "=== " then it is option field
        if (line.startsWith("=== ")) {
            optName = line.substring(3).trim();
            JSONObject field = new JSONObject();
            doc.put(optName, field);
        }
        else {
            Object curObject = doc.get(optName);
            if (curObject != null) {
                JSONObject curOpt = (JSONObject) curObject;

                if (line.startsWith("* ")) {
                    String field = line.substring(1).trim();
                    String[] parts = field.split(":");
                    if (parts.length > 1) {
                        String k = parts[0];
                        String v = "";
                        for (int i = 1; i < parts.length; i++) {
                            if (!v.isEmpty()) {
                                v += ":";
                            }
                            v += parts[i].replaceAll("`", "");
                        }
                        curOpt.put(k.trim(), v.trim());
                    }
                }
                else if (line.startsWith("** ")) // must be list item
                {
                    // Try to get default list object for current option
                    String listKey = "Default List";
                    Object o = curOpt.get(listKey);
                    JSONObject defList = new JSONObject();
                    if (o != null) {
                        defList = (JSONObject) o;
                    }
                    else {
                        curOpt.put(listKey, defList); // create new if not exist
                    }

                    String curLine = line.substring(2).trim();

                    // try getting item description
                    String[] parts = curLine.split("` - ");
                    if (parts.length > 0) {
                        String k = parts[0].replaceAll("`", "").trim();
                        String v = "";
                        if (parts.length > 1) // there is description
                        {
                            for (int i = 1; i < parts.length; i++) {
                                if (!v.isEmpty()) {
                                    v += "` - ";
                                }
                                v += parts[i];
                            }
                        }
                        defList.put(k.trim(), v.trim());
                    }
                }
                else
                // must be description
                {
                    String curLine = line.trim();
                    if (!curLine.isEmpty()) {
                        String k = "Description";
                        Object o = curOpt.get(k);
                        if (o == null) {
                            curOpt.put(k, curLine);
                        }
                        else {
                            String v = curOpt.get(k) + " " + curLine;
                            curOpt.put(k, v.trim());
                        }
                    }
                }
            }
        }

        return optName;
    }

    private void parseAsciidoc() throws Exception {
        try (FileInputStream fstream = new FileInputStream(homeFolder + "/" + asciidocPath);
             InputStreamReader istream = new InputStreamReader(fstream);
             BufferedReader br = new BufferedReader(istream)) {

            String strLine;
            String curOptName = null;

            // Read File Line By Line
            while ((strLine = br.readLine()) != null) {
                // Print the content on the console
                curOptName = parseLine(strLine, curOptName);
            }
        }
    }
}
