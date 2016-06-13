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
package hoot.services.controllers.job;

import java.io.IOException;
import java.lang.reflect.Method;
import java.sql.Connection;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;

import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.support.ClassPathXmlApplicationContext;

import hoot.services.HootProperties;
import hoot.services.db.DbUtils;
import hoot.services.db2.JobStatus;
import hoot.services.job.JobStatusManager;
import hoot.services.job.JobStatusManager.JOB_STATUS;
import hoot.services.nativeInterfaces.JobExecutionManager;
import hoot.services.utils.ResourceErrorHandler;
import hoot.services.utils.ResourcesCleanUtil;
import hoot.services.validators.job.JobFieldsValidator;


/**
 * @author Jong Choi
 *
 *         Servlet class for handling job execution internally.
 *
 */
@Path("")
public class JobResource {
    private static final Logger logger = LoggerFactory.getLogger(JobResource.class);
    private static final ClassPathXmlApplicationContext appContext;
    private static final long chainJosStatusPingInterval;

    // Thread pool for chain and job processor
    private static final ExecutorService jobThreadExecutor;
    private static final JobExecutionManager jobExecMan;

    private final JSONObject jobInfo = new JSONObject();
    private final JSONArray childrenInfo = new JSONArray();


    static {
        appContext = new ClassPathXmlApplicationContext("hoot/spring/CoreServiceContext.xml");
        jobExecMan = ((JobExecutionManager) appContext.getBean("jobExecutionManagerNative"));

        Long value = Long.parseLong(HootProperties.getProperty("chainJosStatusPingInterval"));

        chainJosStatusPingInterval = (value < 1000) ? 1000 : value;

        int threadpoolSize = 5;
        try {
            threadpoolSize = Integer.parseInt(HootProperties.getProperty("internalJobThreadSize"));
        }
        catch (Exception ex) {
            logger.error("Failed to get internalJobThreadSize. Setting threadpool size to 5.");
        }
        logger.debug("Threadpool Acquire");

        jobThreadExecutor = Executors.newFixedThreadPool(threadpoolSize);
    }

    /**
     * Constructor. execManager is the one that handles the execution through
     * configured Native Interface.
     *
     * @throws IOException
     * @throws NumberFormatException
     */
    public JobResource() {
    }

    @POST
    @Path("/chain/{jobid}")
    @Consumes(MediaType.TEXT_PLAIN)
    @Produces(MediaType.TEXT_PLAIN)
    public Response processChainJob(@PathParam("jobid") String jobId, String jobs) throws Exception {
        logger.debug("Curent JobResource thread count:{}", ((ThreadPoolExecutor) jobThreadExecutor).getActiveCount());

        initJob(jobId);

        Runnable chainJobWorker = new ProcessChainJobWorkerThread(jobId, jobs);
        jobThreadExecutor.execute(chainJobWorker);

        return Response.ok().build();
    }

    private class ProcessChainJobWorkerThread implements Runnable {
        private final String jobId;
        private final String jobs;

        public ProcessChainJobWorkerThread(String jobid, String jobParams) {
            jobId = jobid;
            jobs = jobParams;
        }

        @Override
        public void run() {
            processCommand();
        }

        private void processCommand() {

            logger.debug("Start chain job: {}", jobId);
            Connection conn = DbUtils.createConnection();

            jobInfo.put("chainjobstatus", jobId);

            JobStatusManager jobStatusManager = null;
            JSONObject childJobInfo = null;
            try {
                jobStatusManager = new JobStatusManager(conn);

                JSONParser parser = new JSONParser();
                JSONArray chain = (JSONArray) parser.parse(jobs);

                int nChain = chain.size();
                jobInfo.put("childrencount", String.valueOf(nChain));

                for (Object aChain : chain) {
                    String internalJobId = UUID.randomUUID().toString();

                    // prep child job

                    JSONObject job = (JSONObject) aChain;
                    String excType = job.get("exectype").toString();
                    String warnings = null;

                    if (excType.equalsIgnoreCase("reflection")) {
                        // getting jobInfo from inside since it generates job id.
                        childJobInfo = execReflection(jobId, job, jobStatusManager);
                        Object oWarn = childJobInfo.get("warnings");
                        if (oWarn != null) {
                            warnings = oWarn.toString();
                        }
                    }
                    else if (excType.equalsIgnoreCase("reflection_sync")) {
                        childJobInfo = execReflectionSync(jobId, internalJobId, job, jobStatusManager);
                    }
                    else {
                        // create and set info
                        childJobInfo = createChildInfo(internalJobId, JOB_STATUS.RUNNING.toString());
                        setJobInfo(jobInfo, childJobInfo, childrenInfo, JOB_STATUS.RUNNING.toString(), "processing");
                        jobStatusManager.updateJob(jobId, jobInfo.toString());

                        JSONObject result = processJob(internalJobId, job);

                        // try to get warning
                        Object oWarn = result.get("warnings");
                        if (oWarn != null) {
                            warnings = oWarn.toString();
                        }
                    }

                    // if we have warnings then pass on
                    String resDetail = "success";
                    if (warnings != null) {
                        resDetail = "WARNINGS: " + warnings;
                    }

                    setJobInfo(jobInfo, childJobInfo, childrenInfo, JOB_STATUS.COMPLETE.toString(), resDetail);

                    jobStatusManager.updateJob(jobId, jobInfo.toString());
                }

                jobStatusManager.setComplete(jobId, jobInfo.toString());
            }
            catch (Exception ex) {
                if (jobStatusManager != null) {
                    if (childJobInfo != null) {
                        setJobInfo(jobInfo, childJobInfo, childrenInfo, JOB_STATUS.FAILED.toString(), ex.getMessage());
                        logger.error(ex.getMessage());
                    }
                    jobStatusManager.setFailed(jobId, jobInfo.toString());
                }
            }
            finally {
                logger.debug("End process chain Job: {}", jobId);
                try {
                    DbUtils.closeConnection(conn);
                }
                catch (Exception ex) {
                    logger.error(ex.getMessage());
                }
            }
        }
    }

    // this function is currently used by WfsManager only and WfsManager does
    // not perform any job tracking. It is raw class.
    private JSONObject execReflectionSync(String jobId, String childJobId, JSONObject job,
            JobStatusManager jobStatusManager) throws Exception {
        String className = job.get("class").toString();
        String methodName = job.get("method").toString();

        String internalJobId = (job.get("internaljobid") == null) ? null : job.get("internaljobid").toString();

        JSONArray paramsList = (JSONArray) job.get("params");

        Class<?>[] paramTypes = new Class[paramsList.size()];
        Object[] parameters = new Object[paramsList.size()];
        for (int i = 0; i < paramsList.size(); i++) {
            JSONObject param = (JSONObject) paramsList.get(i);
            String paramType = param.get("paramtype").toString();
            Object oIsPrim = param.get("isprimitivetype");
            if ((oIsPrim != null) && oIsPrim.toString().equalsIgnoreCase("true")) {
                Class<?> classWrapper = Class.forName(paramType);
                paramTypes[i] = (Class<?>) classWrapper.getField("TYPE").get(null);
            }
            else {
                paramTypes[i] = Class.forName(paramType);
            }
            parameters[i] = param.get("value");
        }

        Class<?> c = Class.forName(className);
        Object instance = c.newInstance();

        JSONObject childJobInfo = null;
        String currentChildJobId = childJobId;

        // May be we would need create interface to guarranttee that it will
        // return a job id?  Add internal job id to end of method call
        if (internalJobId != null) {
            currentChildJobId = internalJobId;
            childJobInfo = createChildInfo(currentChildJobId, JOB_STATUS.RUNNING.toString());
            setJobInfo(jobInfo, childJobInfo, childrenInfo, JOB_STATUS.RUNNING.toString(), "processing");
            jobStatusManager.updateJob(jobId, jobInfo.toString());

            Object[] newParams = new Object[paramsList.size() + 1];
            System.arraycopy(parameters, 0, newParams, 0, parameters.length);
            newParams[parameters.length] = internalJobId;

            Class<?>[] newParamTypes = new Class[paramsList.size() + 1];
            System.arraycopy(paramTypes, 0, newParamTypes, 0, paramsList.size());
            newParamTypes[parameters.length] = String.class;
            Method m = c.getDeclaredMethod(methodName, newParamTypes);
            // This will blow if the method is not designed to handle job id
            m.invoke(instance, newParams);
        }
        else {
            Method m = c.getDeclaredMethod(methodName, paramTypes);
            Object oReflectJobId = m.invoke(instance, parameters);
            if (oReflectJobId != null) {
                currentChildJobId = oReflectJobId.toString();
            }
            // Updating job status info. Looks like we need to wait till job is
            // done to get job id.
            // With this we can not canel..
            childJobInfo = createChildInfo(currentChildJobId, JobStatusManager.JOB_STATUS.RUNNING.toString());
            setJobInfo(jobInfo, childJobInfo, childrenInfo, JOB_STATUS.RUNNING.toString(), "processing");
            jobStatusManager.updateJob(jobId, jobInfo.toString());
        }

        return childJobInfo;
    }

    public JSONObject execReflection(String jobId, JSONObject job, JobStatusManager jobStatusManager)
            throws Exception {
        JSONObject childJobInfo = execReflectionSync(jobId, null, job, jobStatusManager);
        Object oReflectJobId = childJobInfo.get("id");

        if (oReflectJobId != null) {
            String reflectionJobId = oReflectJobId.toString();
            boolean isDone = false;
            // If we do not get any Job Status then try 10 time and give up
            int nRetryCnt = 0;
            while (!isDone) {
                JSONObject status = getJobStatusObj(reflectionJobId);
                if (status != null) {
                    String currStat = status.get("status").toString();
                    if (!currStat.equalsIgnoreCase("running")) {
                        isDone = true;
                        if (currStat.equals(JOB_STATUS.FAILED.toString())) {
                            throw new Exception(status.get("statusDetail").toString());
                        }
                        if (status.get("statusDetail") != null) {
                            childJobInfo.put("warnings", status.get("statusDetail").toString());
                            childJobInfo.put("detail", "warning");
                            childJobInfo.put("status", JOB_STATUS.COMPLETE.toString());
                        }
                    }
                }
                else {
                    try {
                        Thread.sleep(1000);
                    }
                    catch (InterruptedException e) {
                        isDone = true;
                    }

                    if (nRetryCnt > 10) {
                        // isDone = true;
                    }
                    nRetryCnt++;
                }
                if (!isDone) {
                    try {
                        Thread.sleep(chainJosStatusPingInterval);
                    }
                    catch (InterruptedException e) {
                        //
                    }
                }
            }
        }

        return childJobInfo;
    }

    /**
     * Processes requested job. Parameter is in String in JSON format
     *
     * @throws SQLException
     */
    @POST
    @Path("/{jobid}")
    @Consumes(MediaType.TEXT_PLAIN)
    @Produces(MediaType.TEXT_PLAIN)
    public Response processJob(@PathParam("jobid") String jobId, String params) throws Exception {
        logger.debug("Curent JobResource thread count:{}", ((ThreadPoolExecutor) jobThreadExecutor).getActiveCount());

        initJob(jobId);

        Runnable jobWorker = new ProcessJobWorkerThread(jobId, params);

        jobThreadExecutor.execute(jobWorker);

        return Response.ok().build();
    }

    /**
     * process job worker thread. Spawns this thread to return right away to
     * client while keeping open thread count low. Job Status will be updated
     * while running as thread.
     *
     */
    public class ProcessJobWorkerThread implements Runnable {
        private final String jobId;
        private final String params;

        public ProcessJobWorkerThread(String jobid, String jobParams) {
            jobId = jobid;
            params = jobParams;
        }

        @Override
        public void run() {
            processCommand();
        }

        private void processCommand() {

            logger.debug("Processing job: {}", jobId);
            Connection conn = DbUtils.createConnection();
            JobStatusManager jobStatusManager = null;
            try {
                jobStatusManager = new JobStatusManager(conn);

                JSONParser parser = new JSONParser();
                JSONObject command = (JSONObject) parser.parse(params);

                JSONObject result = processJob(jobId, command);

                String warnings = null;
                Object oWarn = result.get("warnings");
                if (oWarn != null) {
                    warnings = oWarn.toString();
                }

                if (warnings == null) {
                    jobStatusManager.setComplete(jobId);
                }
                else {
                    jobStatusManager.setComplete(jobId, "WARNINGS: " + warnings);
                }
            }
            catch (Exception e) {
                if (jobStatusManager != null) {
                    jobStatusManager.setFailed(jobId, e.getMessage());
                    logger.error(e.getMessage());
                }
            }
            finally {
                logger.debug("End process Job: {}", jobId);
                try {
                    DbUtils.closeConnection(conn);
                }
                catch (Exception ex) {
                    logger.error(ex.getMessage());
                }
            }
        }
    }

    private static JSONObject processJob(String jobId, JSONObject command) throws Exception {
        logger.debug("processing Job: {}", jobId);
        command.put("jobId", jobId);

        String resourceName = command.get("caller").toString();
        JobFieldsValidator validator = new JobFieldsValidator(resourceName);
        JSONArray paramsList = (JSONArray) command.get("params");

        Map<String, String> paramsMap = new HashMap<>();
        for (Object aParamsList : paramsList) {
            JSONObject o = (JSONObject) aParamsList;
            for (Object o1 : o.entrySet()) {
                Map.Entry mEntry = (Map.Entry) o1;
                String key = (String) mEntry.getKey();
                String val = (String) mEntry.getValue();
                paramsMap.put(key, val);
            }
        }

        List<String> missingList = new ArrayList<>();
        if (!validator.validateRequiredExists(paramsMap, missingList)) {
            logger.error("Missing following required field(s): {}", missingList);
        }

        logger.debug("calling native request Job: {}", jobId);

        return jobExecMan.exec(command);
    }

    /**
     * Raw call to terminate job
     *
     * @throws Exception
     */
    public static void terminateJob(String childId) throws Exception {
        jobExecMan.terminate(childId);
    }

    /**
     * Terminate Job and its children jobs
     */
    public static String terminateJob(String jobId, String mapId) throws Exception {
        /*
         * Example job status
         *
         * {"jobId":"dae4be8a-4964-4a9a-8d7d-4e8e738a5b58","statusDetail":
         * "{\"chainjobstatus\":\"dae4be8a-4964-4a9a-8d7d-4e8e738a5b58\",
         * \"children\":[{\"id\":\"e16282c6-2432-4a45-a582-b0dd3f2f0d9f\",\"detail\":\"success\",\"status\":\"complete\"
         * }, {\
         * "id\":\"66d53cfc-29b6-49eb-9459-9a74794d34b2\",\"detail\":\"success\",\"status\":\"complete\"
         * }, {\
         * "id\":\"43821fd7-a488-4137-b25a-2a66d0a4e197\",\"detail\":\"success\",\"status\":\"complete\"}]}"
         * ,"status":"complete"}
         */
        JSONParser parser = new JSONParser();
        // see if chain job
        JSONObject status = getJobStatusObj(jobId);
        if (status != null) {
            String detailStr = status.get("statusDetail").toString();
            JSONObject detail = (JSONObject) parser.parse(detailStr);
            if (detail != null) {
                if (detail.containsKey("chainjobstatus") && detail.containsKey("children")) {
                    if (detail.containsKey("mapid")) {
                        mapId = detail.get("mapid").toString();
                    }

                    JSONArray children = (JSONArray) detail.get("children");
                    if (children != null) {
                        for (Object aChildren : children) {
                            JSONObject child = (JSONObject) aChildren;
                            if (child.get("status") != null) {
                                String childStat = child.get("status").toString();
                                if (childStat.equals(JOB_STATUS.RUNNING.toString())) {
                                    if (child.get("id") != null) {
                                        String childId = child.get("id").toString();
                                        terminateJob(childId);
                                    }
                                }
                            }
                        }

                        if (mapId != null) {
                            ResourcesCleanUtil clean = new ResourcesCleanUtil();
                            clean.deleteLayers(mapId);
                        }
                    }
                }
            }
        }

        return jobId;
    }

    private static String getProgressText(String jobId) throws Exception {
        return jobExecMan.getProgress(jobId);
    }

    /**
     * This service allows for executing Hootenanny tasks and tracking the
     * status of Hootenanny jobs launched by other web services. Not all
     * Hootenanny web services create jobs which can be tracked by this service.
     * 
     * GET hoot-services/job/status/{Job Id}
     * 
     * @param jobId
     *            id of the job to track
     * @return job status JSON
     */
    @GET
    @Path("/status/{jobId}")
    @Produces(MediaType.APPLICATION_JSON)
    public static Response getJobStatus(@PathParam("jobId") String jobId) {
        String outStr = "unknown";
        try {
            JSONObject status = getJobStatusObj(jobId);

            try {
                String detail = status.get("statusDetail").toString();
                if ((detail != null) && (!detail.trim().isEmpty()) && detail.trim().startsWith("{")) {
                    JSONParser p = new JSONParser();
                    JSONObject stat = (JSONObject) p.parse(detail);

                    // for now we will check for chain job only since ogr2osm is called for such job
                    if (stat.get("chainjobstatus") != null) {
                        String strCnt = stat.get("childrencount").toString();
                        int nCnt = Integer.parseInt(strCnt);
                        int maxPartPercent = 100 / nCnt;

                        int nComplete = 0;

                        Map<String, Integer> progMap = new LinkedHashMap<>();

                        JSONArray children = (JSONArray) stat.get("children");

                        String lastLine = "";
                        for (Object aChildren : children) {
                            JSONObject child = (JSONObject) aChildren;
                            String childId = child.get("id").toString();
                            if (child.get("status").toString().endsWith("running")) {

                                String progress = getProgressText(childId);
                                String[] lines = progress.split("\n");

                                Integer childProg = progMap.get(childId);
                                if (childProg == null) {
                                    childProg = 0;
                                    progMap.put(childId, 0);
                                }
                                for (String line : lines) {
                                    String currLine = line.trim();
                                    if (currLine.startsWith("{")) {
                                        JSONObject progInfo = (JSONObject) p.parse(currLine);
                                        JSONObject progStatus = (JSONObject) progInfo.get("status");
                                        if (progStatus.get("jobFinished").toString().equals("true")) {
                                            // If completed before we can get the progress info.
                                            // (This happens because the job status ping time may
                                            // be longer than the whole job)
                                            childProg = maxPartPercent;
                                        }
                                        else {
                                            long currPerc = (Long) progStatus.get("percentComplete");

                                            double dMxPartPerc = maxPartPercent;
                                            double dCurrPerc = currPerc;
                                            childProg = (int) (dMxPartPerc * (dCurrPerc / 100));
                                        }
                                    }
                                    progMap.put(childId, childProg);
                                    lastLine = currLine;
                                }
                            }
                            else {
                                progMap.put(childId, maxPartPercent);
                                nComplete++;
                            }
                        }

                        int totalPercent = 0;
                        if (nComplete == nCnt) {
                            totalPercent = 100;
                        }
                        else {
                            for (Map.Entry<String, Integer> entry : progMap.entrySet()) {
                                totalPercent += entry.getValue();
                            }
                        }

                        status.put("percentcomplete", totalPercent);
                        status.put("lasttext", lastLine);
                    }
                }
            }
            catch (Exception ex) {
                // if something goes wrong we will not put progress
                logger.error("Error during job status retrieval!", ex);
            }

            outStr = status.toJSONString();
        }
        catch (Exception e) {
            ResourceErrorHandler.handleError(
                    "Error retrieving job status for job: " + jobId + " Error: " + e.getMessage(),
                    Status.INTERNAL_SERVER_ERROR, logger);
        }

        return Response.ok(outStr, MediaType.APPLICATION_JSON).build();
    }

    /**
     * Return job status
     */
    public static JSONObject getJobStatusObj(String jobId) throws Exception {
        JSONObject status = new JSONObject();
        Connection conn = DbUtils.createConnection();

        try {
            JobStatusManager jobStatusManager = new JobStatusManager(conn);

            status.put("jobId", jobId);
            JobStatus jobStatusObj = jobStatusManager.getJobStatusObj(jobId);
            if (jobStatusObj == null) {
                return null;
            }

            JOB_STATUS stat = JOB_STATUS.fromInteger(jobStatusObj.getStatus());
            if (stat == null) {
                stat = JOB_STATUS.UNKNOWN;
            }

            status.put("status", stat.toString());
            status.put("statusDetail", jobStatusObj.getStatusDetail());
        }
        finally {
            DbUtils.closeConnection(conn);
        }

        return status;
    }

    private static void setJobInfo(JSONObject jobInfo, JSONObject child, JSONArray children, String stat, String detail) {
        for (Object aChildren : children) {
            JSONObject c = (JSONObject) aChildren;
            if (c.get("id").toString().equals(child.get("id").toString())) {
                c.put("status", stat);
                c.put("detail", detail);
                return;
            }
        }

        child.put("status", stat);
        child.put("detail", detail);
        children.add(child);
        jobInfo.put("children", children);
    }

    private static JSONObject createChildInfo(String id, String stat) {
        JSONObject child = new JSONObject();
        child.put("id", id);
        child.put("status", stat);

        return child;
    }

    private static void initJob(String jobId) throws Exception {
        Connection conn = DbUtils.createConnection();
        JobStatusManager jobStatusManager = new JobStatusManager(conn);
        jobStatusManager.addJob(jobId);
        DbUtils.closeConnection(conn);;
    }
}
