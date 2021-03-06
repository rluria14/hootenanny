-- Add a test user for tests
INSERT INTO users(email,id,display_name,provider_access_key,provider_access_token,hootservices_last_authorize,hootservices_created_at,provider_created_at,privileges) VALUES ('karma@test.com',-1541432234,'Karma','H0xC4KbUNRIUBJ8HrJIpdGz4WfouNiS0Lh1Yd50z','miYcdFFakicoYH7Xkw6Sop0mSM1fCzlRWK8eaHoL',now(),now(),now(),'"advanced"=>"true"');


-- Add a spring session for our test user
INSERT INTO spring_session(session_id,creation_time,last_access_time,max_inactive_interval,principal_name,user_id) VALUES ('ff47f751-c831-41ee-800f-5ef8b9371ee3',cast(extract(epoch from (now())) as bigint)*1000,cast(extract(epoch from (now() + INTERVAL '1 second')) as bigint)*1000,31536000,'',-1541432234);


-- Add rememberme attribute data for the spring session
INSERT INTO spring_session_attributes(session_id,attribute_name,attribute_bytes) VALUES ('ff47f751-c831-41ee-800f-5ef8b9371ee3','org.springframework.security.oauth.consumer.rememberme.HttpSessionOAuthRememberMeServices#REMEMBERED_TOKENS','\xaced0005737200116a6176612e7574696c2e486173684d61700507dac1c31660d103000246000a6c6f6164466163746f724900097468726573686f6c6478703f400000000000007708000000100000000078');


-- Add openStreetMap attribute data for the spring session
INSERT INTO spring_session_attributes(session_id,attribute_name,attribute_bytes) VALUES ('ff47f751-c831-41ee-800f-5ef8b9371ee3','OAUTH_TOKEN#openStreetMap','\xaced00057372003e6f72672e737072696e676672616d65776f726b2e73656375726974792e6f617574682e636f6e73756d65722e4f41757468436f6e73756d6572546f6b656ec7af2259bac92fe90200055a000b616363657373546f6b656e4c00146164646974696f6e616c506172616d657465727374000f4c6a6176612f7574696c2f4d61703b4c000a7265736f7572636549647400124c6a6176612f6c616e672f537472696e673b4c000673656372657471007e00024c000576616c756571007e00027870017074000d6f70656e5374726565744d61707400286d695963644646616b69636f594837586b7736536f70306d534d3166437a6c52574b386561486f4c74002848307843344b62554e524955424a3848724a497064477a3457666f754e6953304c6831596435307a');
