/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,		Laurent De Marez <laurentdemarez@gmail.com>
 *
 * This file is part of Gfire.
 *
 * Gfire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gfire.h"

#include "gf_network.h"
#include "gf_packet.h"
#include "gf_chat.h"

#include "cipher.h"



void gfire_add_header(guint8 *packet, int length, int type, int atts)
{
	guint8 buffer[5] = { 0,0,0,0,(guint8) atts };
	guint16 l = GUINT16_TO_LE((guint16) length);
	guint16 t = GUINT16_TO_LE((guint16) type);

	memcpy(buffer,&l,2);
	memcpy(buffer+2,&t,2);
	memcpy(packet,buffer,XFIRE_HEADER_LEN);
}


int gfire_add_att_name(guint8 *packet,int packet_length, char *att)
{
	packet[packet_length] = (guint8)strlen(att);//set att length
	memcpy(packet+packet_length+1,att,strlen(att)); //set attname
	return packet_length+1+strlen(att);
}

int gfire_read_attrib(GList **values, guint8 *buffer, int packet_len, const char *name,
					gboolean dynamic, gboolean binary, int bytes_to_first, int bytes_between, int vallen)
{
	int index = 0; int i=0; int ali = 0; int alen = 0;
	gchar tmp[100];
	guint16 numitems = 0; guint16 attr_len = 0;
	guint8 *str;

	memset(tmp, 0x00, 100);


	if (NULL == name) {
		/* don't look for attribute name just proccess the items */
		memcpy(&numitems, buffer + index, 2);
		numitems = GUINT16_FROM_LE(numitems);
		index += 2;
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Looking for %d items's in packet.\n", numitems);
	} else {
		alen = strlen(name);
		memcpy(tmp, buffer + index, alen);
		index += strlen(name);
		if ( 0 == g_ascii_strcasecmp(name, tmp)) {
			index += 2;
			memcpy(&numitems, buffer + index, 2);
			numitems = GUINT16_FROM_LE(numitems);
			index += 2;
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "Looking for %d %s's in packet.\n", numitems, NN(name));
		} else {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: %s signature isn't in the correct position.\n", NN(name));
			return -1;
		} 
	}
	/* if we are copying a string make sure we have a space for the trailing \0 */
	if (binary) ali = 0;
		else ali = 1;

	for (i = 0; i < numitems; i++) {
		if (dynamic) {
			memcpy(&attr_len, buffer + index, 2);
			attr_len = GUINT16_FROM_LE(attr_len); index+=2;
		} else attr_len = vallen;

		if (dynamic && (attr_len == 0)) str = NULL;
			else {
				str = g_malloc0(sizeof(char) * (attr_len+ali));
				memcpy(str, buffer + index, attr_len);
				if (!binary) str[attr_len] = 0x00;
			}
		index += attr_len;
		*values = g_list_append(*values,(gpointer *)str);
		if ( index > packet_len ) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 131: more friends then packet length.\n");
			return -1;
		}
	}
	return index;
}


static void hashSha1(char* input,char* digest)
//based on code from purple_util_get_image_filename in the pidgin 2.2.0 source
{
	PurpleCipherContext *context;                                                                                                                                            
		                                                                                                     
		context = purple_cipher_context_new_by_name("sha1", NULL);                                   
		if (context == NULL)  {                                                                                            
			purple_debug_error("util", "Could not find sha1 cipher\n");                          
			g_return_val_if_reached(NULL);                                                       
		}	                                                                                            
										                                                                                                     
	                                                                    
		purple_cipher_context_append(context, input, strlen(input));                                
		if (!purple_cipher_context_digest_to_str(context, 41, digest, NULL)){                                                                                            
	    		purple_debug_error("util", "Failed to get SHA-1 digest.\n");                         
	    		g_return_val_if_reached(NULL);                                                       
		}                                                                                            
		purple_cipher_context_destroy(context);                                                      
		digest[40]=0;
}
	

int gfire_send_auth(PurpleConnection *gc, int packet_len, int packet_id)
{		

	char *passwd = (char *)purple_account_get_password(gc->account);
	char *name = (char *)purple_account_get_username(gc->account);
	char salt[41]; 					/*the salt we got from the server*/
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	char secret[] = "UltimateArena";		 /*Secret string that is used to hash the passwd*/	
	char sha_string[41];
	char hash_it[100];
	char hash_final[81];
	int index = 0;

	/*
	 * packet_length 00 type(01) 00 numberOfAtts
	 * attribute_length 'name'  usernameLength_length usernameLength 00 username
	 * attribute_length 'password'  passwdLength_length passwdLength 00 cryptedPassword
	 */

	/* extract the salt from the packet and add a null terminator */
	memcpy(salt,gfire->buff_in+13, 40);
	salt[40]=0x00;
	
	int pkt_len = 97+strlen(name); /*Packet length is 97 + username length*/ 
	
	memset(gfire->buff_out,0x00,GFIRE_BUFFOUT_SIZE);
	gfire_add_header(gfire->buff_out, pkt_len, 1, 3); /*add header*/ 
	index += 5;
	
	index = gfire_add_att_name(gfire->buff_out,index, "name");/*add name*/
	gfire->buff_out[index++] = 0x01; 			/*username length length*/
	gfire->buff_out[index++] = (char)strlen(name); 	/*username length*/
	gfire->buff_out[index++] = 0x00;
	
	memcpy(gfire->buff_out+index, name, strlen(name)); 	/* add username */
	index += strlen(name);
	
	index = gfire_add_att_name(gfire->buff_out,index, "password");
	gfire->buff_out[index++] = 0x01; 			/*hashed passwd length length*/
	gfire->buff_out[index++] = 0x28; 			/*hashed passwd length, always 40 (SHA1)*/
	gfire->buff_out[index++] = 0x00;
	
	hash_it[0] = 0;
	strcat(hash_it,name);				/*create string: name+passwd+secret*/
	strcat(hash_it,passwd);
	strcat(hash_it,secret);

	hashSha1(hash_it,hash_final);
	memcpy(hash_final+40,salt,40);			/* mix it with the salt and rehash*/
	
	hash_final[80] = 0x00; 				/*terminate the string*/

	hashSha1(hash_final,sha_string);
			
	memcpy(gfire->buff_out+index,sha_string,strlen(sha_string));/*insert the hash of the passwd*/
	index += strlen(sha_string);
	
	/* added 09-08-2005 difference in login packet */ 
	index = gfire_add_att_name(gfire->buff_out,index, "flags");/*add flags*/ 
	gfire->buff_out[index++]=0x02; 
	
	// run memset once, fill 25 char's with 0's this is from a packet capture 
	// they tack on "flags" + 4 bytes that are 0x00 + "sid" + 16 bytes that are 0x00 
	
	index+=4; 
	index = gfire_add_att_name(gfire->buff_out,index, "sid");/*add sid*/ 
	gfire->buff_out[index++] = 0x03; 
	
	// rest of packet is 16 bytes filled with 0x00 
	index+= 16; 
	
	return index;
}


/* reads session information from intial login grabs our info */
void gfire_packet_130(PurpleConnection *gc, int packet_len)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN + 1;
	char tmp[100] = "";
	guint16 slen = 0;


	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;
	memcpy(tmp, gfire->buff_in + index, strlen("userid"));
	tmp[strlen("userid")] = 0x00;
	index += strlen("userid") + 1;
	if (0 == g_ascii_strcasecmp("userid", tmp)) {
		if (!gfire->userid) g_free(gfire->userid);
		gfire->userid = g_malloc0(XFIRE_USERID_LEN);
		memcpy(gfire->userid, gfire->buff_in + index, XFIRE_USERID_LEN);
		index += XFIRE_USERID_LEN + 1;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 130: userid not in correct position.\n");
		return;
	}
	index += 4;
	if (!gfire->sid) g_free(gfire->sid);
	gfire->sid = g_malloc0(XFIRE_SID_LEN);
	memcpy(gfire->sid, gfire->buff_in + index, XFIRE_SID_LEN);
	index += XFIRE_SID_LEN +6;

	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	index += sizeof(slen);
	slen = GUINT16_FROM_LE(slen);
	if (NULL != gfire->alias) g_free(gfire->alias);
	gfire->alias = g_malloc0(slen + 1);
	memcpy(gfire->alias, gfire->buff_in + index, slen);
	if (slen > 0) gfire->alias[slen] = 0x00;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(session): Our userid = %02x%02x%02x%02x, Our Alias = %s\n",
			NNA(gfire->userid, gfire->userid[0]), NNA(gfire->userid, gfire->userid[1]),
			NNA(gfire->userid, gfire->userid[2]), NNA(gfire->userid, gfire->userid[3]), NN(gfire->alias) );
// FIXME("gfire_packet_130");
}



/* reads buddy list from server and populates purple blist */
void gfire_packet_131(PurpleConnection *gc, int packet_len)
{
	int index, attrib, i = 0;
	gfire_buddy *gf_buddy = NULL;
	GList *friends = NULL;
	GList *nicks = NULL;
	GList *userids = NULL;
	GList *f, *n, *u;
	gchar uids[(XFIRE_USERID_LEN * 2) + 1];
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	if (packet_len < 16) {
		purple_debug_error("gfire", "packet 131 received, but too short. (%d bytes)\n", packet_len);
		return;
	}

	index = 6;
	attrib = gfire_read_attrib(&friends, gfire->buff_in + index, packet_len - index, "friends", TRUE, FALSE, 0, 0, 0);
	if (attrib < 1 && friends != NULL) return;

	index += attrib + 1;
	attrib = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, "nick", TRUE, FALSE, 0, 0, 0);
	if (attrib < 1 ) {
		if (friends != NULL) g_list_free(friends);
		if (nicks != NULL) g_list_free(nicks);
		return;
	}

	index += attrib + 1;	
	attrib = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, "userid", FALSE, TRUE, 0, 0, XFIRE_USERID_LEN);
	if (attrib < 1 ) {
		if (friends != NULL) g_list_free(friends);
		if (nicks != NULL) g_list_free(nicks);
		if (userids != NULL) g_list_free(userids);
		return;
	}

	friends = g_list_first(friends);
	nicks = g_list_first(nicks);
	userids = g_list_first(userids);
	
	f = friends;
	n = nicks;
	u = userids;
	
	while (f != NULL)
	{
		gf_buddy = g_malloc0(sizeof(gfire_buddy));
		gfire->buddies = g_list_append(gfire->buddies, (gpointer *)gf_buddy);

		gf_buddy->name = (gchar *)f->data;
		gf_buddy->alias = (gchar *)n->data;
		gf_buddy->userid = (guint8 *)u->data;
		gf_buddy->friend = (gboolean )TRUE;

		if (gf_buddy->alias == NULL) gf_buddy->alias = g_strdup(gf_buddy->name);

		/* cast userid into string */
		for(i = 0; i < XFIRE_USERID_LEN; i++) g_sprintf(uids + (i * 2), "%02x", gf_buddy->userid[i]);

		uids[(XFIRE_USERID_LEN * 2)] = 0x00;
		gf_buddy->uid_str = g_strdup(uids);

		f->data = NULL;
		u->data = NULL;
		n->data = NULL;

		f = g_list_next(f);
		u = g_list_next(u);
		n = g_list_next(n);
	}

	g_list_free(friends);
	g_list_free(nicks);
	g_list_free(userids);
	
	gfire->blist_loaded = (gboolean )TRUE;

	n = gfire->buddies;
	while (n != NULL)
	{
		gf_buddy = (gfire_buddy *)n->data;
		if (!gf_buddy->friend) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "buddy info: %s, %s, %02x%02x%02x%02x, %s\n",
			     NN(gf_buddy->name), NN(gf_buddy->uid_str), NNA(gf_buddy->userid, gf_buddy->userid[0]),
			     NNA(gf_buddy->userid, gf_buddy->userid[1]), NNA(gf_buddy->userid, gf_buddy->userid[2]),
			     NNA(gf_buddy->userid, gf_buddy->userid[3]), NN(gf_buddy->alias));
		}
		n = g_list_next(n);
	}
}


GList *gfire_read_buddy_online(PurpleConnection *gc, int packet_len)
{
	guchar tmp[100];
	int index = 0;
	int itmp = 0;
	int i = 0;
	guint8 *str = NULL;
	GList *tmp_list = NULL;
	GList *userids = NULL;
	GList *sids = NULL;
	GList *ret = NULL;
	GList *u,*s;
	gfire_buddy *gf_buddy = NULL;
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	memset((void *)tmp, 0x00, 100);

	if (packet_len < 16) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 132 received but way too short?!? %d bytes\n",
			packet_len);
		return NULL;
	}

	index = 6;
	itmp = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, "userid", FALSE, TRUE, 0, 0, 
							XFIRE_USERID_LEN);						
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
		if (sids) g_list_free(sids);
			return NULL;
	}	

	userids = g_list_first(userids);
	sids = g_list_first(sids);
	u = userids; s = sids;

	while ( NULL != u ) {
		tmp_list = gfire_find_buddy_in_list(gfire->buddies, u->data, GFFB_UIDBIN);
		if (NULL == tmp_list) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 132: NULL pointer from buddy find\n") ;
			//mem cleanup code
			if (userids) g_list_free(userids);
			if (sids) g_list_free(sids);
			if (tmp_list) g_list_free(tmp_list);
				return NULL;
		}
		gf_buddy = (gfire_buddy *)tmp_list->data;
		str = (guint8 *) s->data;
		if (NULL != gf_buddy->sid) g_free(gf_buddy->sid); 
		gf_buddy->sid = str;
		/* covert sid to string */
		for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", str[i]);
		tmp[XFIRE_SID_LEN*2]=0x00;
		if (NULL != gf_buddy->sid_str) g_free(gf_buddy->sid_str); 
		gf_buddy->sid_str = g_strdup((gchar *)tmp);
		/* clear away state, xfire presence state isn't persistent (unlike purple) */
		gf_buddy->away = FALSE;
		if (NULL != gf_buddy->away_msg) g_free(gf_buddy->away_msg);
		gf_buddy->away_msg = NULL;
		ret = g_list_append(ret, (gpointer *)gf_buddy);
		g_free(u->data); s->data = NULL; u->data = NULL;
		u = g_list_next(u); s = g_list_next(s);

		if (!gf_buddy->away)
		purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);


		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(on/offline): got info for %s -> %s, %s\n", NN(gf_buddy->name),
					NN(gf_buddy->sid_str), NN(gf_buddy->uid_str));

	}

	g_list_free(userids);
	g_list_free(sids);
	return ret;
}



int gfire_get_im(PurpleConnection *gc, int packet_len)
{
	guint8 sid[16], peermsg;
	gchar tmp[100] = "";
	int index,i = 0;
	guint16 ml = 0;
	guint32 msgtype,imindex = 0;
	gfire_im *im = NULL;
	gfire_data *gfire = (gfire_data*)gc->proto_data;

	if (packet_len < 16) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 133 recv'd but way too short?!? %d bytes\n",
			packet_len);
		return 0;
	}

	index = 6;
	/* check the packet for sid signature */
	memcpy(tmp, gfire->buff_in + index, strlen("sid"));
	tmp[strlen("sid")]=0x00;
	if ( 0 == g_ascii_strcasecmp("sid",(char *)tmp)) {
		index+= strlen("sid") + 1;
		memcpy(&sid, gfire->buff_in + index, XFIRE_SID_LEN);
		index+= XFIRE_SID_LEN + 1;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: sid signature isn't in the correct position.\n");
		return 0;
	} 

	/* get peer message */
	memcpy(tmp, gfire->buff_in + index, strlen("peermsg"));
	tmp[strlen("peermsg")]=0x00;
	if ( 0 == g_ascii_strcasecmp("peermsg",(char *)tmp)) {
		index+= strlen("peermsg") + 1;
		memcpy(&peermsg, gfire->buff_in + index, sizeof(peermsg));
		index+= 2;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: peermsg signature isn't in the correct position.\n");
		return 0;
	} 
	
	/* get messsage type */
	memcpy(tmp, gfire->buff_in + index, strlen("msgtype"));
	tmp[strlen("msgtype")]=0x00;
	if ( 0 == g_ascii_strcasecmp("msgtype",(char *)tmp)) {
		index+= strlen("msgtype") + 1;
		memcpy(&msgtype, gfire->buff_in + index, sizeof(msgtype));
		index+= sizeof(msgtype) + 1;
		msgtype = GUINT32_FROM_LE(msgtype);
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: msgtype signature isn't in the correct position.\n");
		return 0;
	} 

	switch (msgtype)
	{
		case 0:
			/* got an im */
			/* get imindex */
			memcpy(tmp, gfire->buff_in + index, strlen("imindex"));
			tmp[strlen("imindex")]=0x00;
			if ( 0 == g_ascii_strcasecmp("imindex",(char *)tmp)) {
				index+= strlen("imindex") + 1;
				memcpy(&imindex, gfire->buff_in + index, sizeof(imindex));
				index+= sizeof(imindex);
				imindex = GUINT32_FROM_LE(imindex);
			} else {
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
					"ERROR: pkt 133: imindex signature isn't in the correct position.\n");
				return 0;
			} 
			index++;
			if (index > packet_len) {
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: IM packet says IM but packet to short.\n");
				return 0;
			}
			/* get im */
			memcpy(tmp, gfire->buff_in + index, strlen("im"));
			tmp[strlen("im")]=0x00;
			if ( 0 == g_ascii_strcasecmp("im",(char *)tmp)) {
				index+= strlen("im") + 1;
				/* get im length */
				memcpy(&ml, gfire->buff_in + index, sizeof(ml));
				index+= sizeof(ml);
				ml = GUINT16_FROM_LE(ml);
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM: im length = %d\n", ml);
				/* get im string */
				gchar *im_str = g_malloc0(sizeof(gchar) * ml+1);
				memcpy(im_str, gfire->buff_in + index, ml);
				im_str[ml] = 0x00;
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM:(recvd): %s\n", NN(im_str));

				im = g_malloc0(sizeof(gfire_im));
				gfire->im = im;
				im->type = msgtype;
				im->peer = peermsg;
				im->index = imindex;
				im->im_str = im_str;
				/* covert sid to string */
				for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", sid[i]);
				tmp[XFIRE_SID_LEN*2]=0x00;
				im->sid_str = g_strdup(tmp);

				/* make response packet */
				memcpy(gfire->buff_out, gfire->buff_in, packet_len);
				gfire_add_header(gfire->buff_out, 62, 2, 2);
				gfire->buff_out[35] = 0x02;
				gfire->buff_out[45] = 0x01;
				return 62;
			} else {
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
					"ERROR: pkt 133: im signature isn't in the correct position.\n");
				return 0;
			} 
			return 0;
		break;

		case 1:
			/* got an ack packet from a previous IM sent */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM ack packet recieved.\n");
			return 0;
		break;

		case 2:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(im): Got P2P info.\n");
			im = g_malloc0(sizeof(gfire_im));
			gfire->im = im;
			im->type = msgtype;
			im->peer = peermsg;
			im->index = 0;
			im->im_str = NULL;
			/* covert sid to string */
			for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", sid[i]);
			tmp[XFIRE_SID_LEN*2]=0x00;
			im->sid_str = g_strdup(tmp);
			return 0;
		break;

		case 3:
			/* got typing from other user */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(im): Got typing info.\n");
			im = g_malloc0(sizeof(gfire_im));
			gfire->im = im;
			im->type = msgtype;
			im->peer = peermsg;
			im->index = 0;
			im->im_str = NULL;
			/* covert sid to string */
			for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", sid[i]);
			tmp[XFIRE_SID_LEN*2]=0x00;
			im->sid_str = g_strdup(tmp);
			return 0;
		break;

		default:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "unknown IM msgtype = %d.\n", msgtype);
			return 0;
	}	
	return 0;
}


int gfire_create_im(PurpleConnection *gc, gfire_buddy *buddy, const char *msg)
{
	int length = 68+strlen(msg);
	int index = 0;
	gfire_data *gfire = NULL;
	guint32	msgtype = 0;
	guint32 imindex = 0;
	guint16 slen = strlen(msg);

	buddy->im++;
	imindex = GUINT32_TO_LE(buddy->im);
	msgtype = GUINT32_TO_LE(msgtype);
	slen = GUINT16_TO_LE(slen);
	gfire = (gfire_data *)gc->proto_data;	
	gfire_add_header(gfire->buff_out, length, 2, 2);/*add header*/
	index += 5;	

	index = gfire_add_att_name(gfire->buff_out,index, "sid");
	gfire->buff_out[index++] = 0x03;
	memcpy(gfire->buff_out+index,buddy->sid, XFIRE_SID_LEN);
	index += XFIRE_SID_LEN;

	index = gfire_add_att_name(gfire->buff_out,index, "peermsg");
	gfire->buff_out[index++] = 0x05;
	gfire->buff_out[index++] = 0x03;

	index = gfire_add_att_name(gfire->buff_out,index, "msgtype");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &msgtype, sizeof(msgtype));
	index+= sizeof(msgtype);
	
	index = gfire_add_att_name(gfire->buff_out,index, "imindex");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &imindex, sizeof(imindex));
	index+= sizeof(imindex);

	index = gfire_add_att_name(gfire->buff_out,index, "im");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index+= sizeof(slen);
	
	memcpy(gfire->buff_out+index, msg, strlen(msg));
	index += strlen(msg);

	return index;

}


GList *gfire_game_status(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	GList *btmp = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *ret = NULL;
	GList *sids = NULL;
	GList *gameids = NULL;
	GList *gameips = NULL;
	GList *gameports = NULL;
	GList *s, *g, *ip, *gp; 
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameids, gfire->buff_in + index, packet_len - index, "gameid", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEID_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
		if (gameids) g_list_free(gameips);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameips, gfire->buff_in + index, packet_len - index, "gip", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEIP_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
		if (gameids) g_list_free(gameids);
		if (gameips) g_list_free(gameips);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameports, gfire->buff_in + index, packet_len - index, "gport", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEPORT_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
		if (gameids) g_list_free(gameids);
		if (gameips) g_list_free(gameips);
		if (gameports) g_list_free(gameports);
			return NULL;
	}	
	gameids = g_list_first(gameids); sids = g_list_first(sids); gameips = g_list_first(gameips);
	gameports = g_list_first(gameports);
	g = gameids; s = sids; ip = gameips; gp = gameports;
	
	while ( NULL != s ){
		btmp = gfire_find_buddy_in_list(gfire->buddies, s->data, GFFB_SIDBIN);
		if (NULL == btmp) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "WARN: pkt 135: (gameinfo) could not find sid in buddy list.\n");
		} else {
			gf_buddy = (gfire_buddy *)btmp->data;
			memcpy(&(gf_buddy->gameid),g->data, XFIRE_GAMEID_LEN);
			gf_buddy->gameid = GUINT32_FROM_LE(gf_buddy->gameid);
			memcpy(&(gf_buddy->gameport),gp->data, XFIRE_GAMEPORT_LEN);
			gf_buddy->gameport = GUINT32_FROM_LE(gf_buddy->gameport);
			gf_buddy->gameport &= 0xFFFF;
			gf_buddy->gameip = (guint8 *)ip->data;
			ret = g_list_append(ret, (gpointer *)gf_buddy);

			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(gameinfo): %s, is playing %d on %d.%d.%d.%d:%d\n",
						NN(gf_buddy->name), gf_buddy->gameid, NNA(gf_buddy->gameip, gf_buddy->gameip[3]),
						NNA(gf_buddy->gameip, gf_buddy->gameip[2]), NNA(gf_buddy->gameip, gf_buddy->gameip[1]),
						NNA(gf_buddy->gameip, gf_buddy->gameip[0]), gf_buddy->gameport);

		}
		g_free(s->data); g_free(g->data); g_free(gp->data);
		s->data = g->data = gp->data = NULL;
		s = g_list_next(s); g = g_list_next(g); ip = g_list_next(ip); gp = g_list_next(gp);
	}
	
	g_list_free(gameids);
	g_list_free(gameports);
	g_list_free(sids);
	g_list_free(gameips);



	return ret;
}


/*send keep alive packet to the server*/
int gfire_ka_packet_create(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	int index = 0;
	gfire_add_header(gfire->buff_out + index, 26, 13, 2);
	index += 5;
	
	index = gfire_add_att_name(gfire->buff_out,index, "value");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	
	index = gfire_add_att_name(gfire->buff_out,index, "stats");
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	return index;
}

GList *gfire_read_buddy_status(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	GList *btmp = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *ret = NULL;
	GList *sids = NULL;
	GList *msgs = NULL;
	GList *s, *m; 
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&msgs, gfire->buff_in + index, packet_len - index, "msg", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (sids) g_list_free(sids);
		if (msgs) g_list_free(msgs);
			return NULL;
	}	

	msgs = g_list_first(msgs); sids = g_list_first(sids);
	m = msgs; s = sids;
	
	while ( NULL != s ){
		btmp = gfire_find_buddy_in_list(gfire->buddies, s->data, GFFB_SIDBIN);
		if (NULL == btmp) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "WARN: pkt 154: (away status) could not find sid in buddy list.\n");
		} else {
			gf_buddy = (gfire_buddy *)btmp->data;
			if (NULL != m->data) {
				/* got an away message */
				gf_buddy->away = TRUE;
				gf_buddy->away_msg = m->data;
			} else {
				/* no message, user is back */
				gf_buddy->away = FALSE;
				if (NULL != gf_buddy->away_msg) g_free(gf_buddy->away_msg);
				gf_buddy->away_msg = NULL;
			}
			ret = g_list_append(ret, (gpointer *)gf_buddy);

			purple_debug(PURPLE_DEBUG_MISC, "gfire","(away): %s, is away/back with msg %s\n",
						NN(gf_buddy->name), NN(gf_buddy->away_msg));
		}
		g_free(s->data);
		s->data = NULL;
		s = g_list_next(s); m = g_list_next(m);
	}
	g_list_free(msgs);
	g_list_free(sids);
	return ret;
}


GList *gfire_read_invitation(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *names = NULL;
	GList *nicks = NULL;
	GList *msgs = NULL;
	GList *ret = NULL;
	GList *n, *a, *m;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data))
		return NULL;

	itmp = gfire_read_attrib(&names, gfire->buff_in + index, packet_len - index, "name", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (names) g_list_free(names);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, "nick", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (names) g_list_free(names);
		if (nicks) g_list_free(nicks);
			return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&msgs, gfire->buff_in + index, packet_len - index, "msg", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (names) g_list_free(names);
		if (nicks) g_list_free(nicks);
		if (msgs) g_list_free(msgs);
			return NULL;
	}	

	names = g_list_first(names); nicks = g_list_first(nicks); msgs = g_list_first(msgs);
	n = names; a = nicks; m = msgs;
	
	while (NULL != n){
		gf_buddy = g_malloc0(sizeof(gfire_buddy));
		ret = g_list_append(ret, gf_buddy);
		gf_buddy->name = (char *)n->data;
		gf_buddy->alias = (char *)a->data;
		gf_buddy->uid_str = (char *)m->data; /* yeah its ugly but it'll work for this */
		n->data = a->data = m->data = NULL;
		n = g_list_next(n); a = g_list_next(a); m = g_list_next(m);
	}
	g_list_free(names);
	g_list_free(nicks);
	g_list_free(msgs);
	return ret;
}


int gfire_invitation_deny(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;


	/* same as the invitation except packet ID in the header */
	len = gfire_invitation_accept(gc, name);
	gfire_add_header(gfire->buff_out, len, 8, 1);
	return len;
}


int gfire_invitation_accept(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;

	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("name");
	memcpy(gfire->buff_out + index, "name", strlen("name"));
	index += strlen("name");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
	
	gfire_add_header(gfire->buff_out, index, 7, 1);
	return index;
}


int gfire_add_buddy_create(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;

	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("name");
	memcpy(gfire->buff_out + index, "name", strlen("name"));
	index += strlen("name");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
		
	gfire->buff_out[index++] = strlen("msg");
	memcpy(gfire->buff_out + index, "msg", strlen("msg"));
	index += strlen("msg");
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;

	gfire_add_header(gfire->buff_out, index, 6, 2);

	return index;

}


int gfire_remove_buddy_create(PurpleConnection *gc, gfire_buddy *b)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b) return 0;

	gfire_add_header(gfire->buff_out, 17, 9, 1);
	gfire->buff_out[index++] = strlen("userid");
	memcpy(gfire->buff_out + index, "userid", strlen("userid"));
	index += strlen("userid");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, b->userid, 4);
	index += 4;

	return index;
}


/*
*Sends a nickname change to the server
*/
int gfire_create_change_alias(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	if (! name ) name = "";
	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("nick");
	memcpy(gfire->buff_out + index, "nick", strlen("nick"));
	index += strlen("nick");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
	gfire_add_header(gfire->buff_out, index, 14, 1);
	return index;	
}

/*
*Sends the packet when we join a game or leave it (gameid 00 00)
*/
int gfire_join_game_create(PurpleConnection *gc, int game, int port, const char *ip)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint32 gport = port;
	guint32 gameid = game;
	const char nullip[4] = {0x00, 0x00, 0x00, 0x00};
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;
	if (!ip) ip = (char *)&nullip;

	gport = GUINT32_TO_LE(gport);
	gameid = GUINT32_TO_LE(gameid);

	index = gfire_add_att_name(gfire->buff_out,index, "gameid");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &gameid, sizeof(gameid));
	index += sizeof(gameid);	

	index = gfire_add_att_name(gfire->buff_out,index, "gip");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = ip[0];
	gfire->buff_out[index++] = ip[1];
	gfire->buff_out[index++] = ip[2];
	gfire->buff_out[index++] = ip[3];

	index = gfire_add_att_name(gfire->buff_out,index, "gport");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &gport, sizeof(gport));
	index += sizeof(gport);

	gfire_add_header(gfire->buff_out, index, 4, 3);

	return index;
}

int gfire_request_avatar_info(PurpleConnection *gc, gfire_buddy *b)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b) return 0;

	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, b->userid, 4);
	index += 4;

	gfire_add_header(gfire->buff_out, index, 37, 1);

	return index;
}


int gfire_send_typing_packet(PurpleConnection *gc, gfire_buddy *buddy)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint32	msgtype = 3;
	guint32 imindex = 0;

	buddy->im++;
	imindex = GUINT32_TO_LE(buddy->im);
	msgtype = GUINT32_TO_LE(msgtype);
	gfire = (gfire_data *)gc->proto_data;	

	index = gfire_add_att_name(gfire->buff_out,index, "sid");
	gfire->buff_out[index++] = 0x03;
	memcpy(gfire->buff_out+index,buddy->sid, XFIRE_SID_LEN);
	index += XFIRE_SID_LEN;

	index = gfire_add_att_name(gfire->buff_out,index, "peermsg");
	gfire->buff_out[index++] = 0x05;
	gfire->buff_out[index++] = 0x03;

	index = gfire_add_att_name(gfire->buff_out,index, "msgtype");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &msgtype, sizeof(msgtype));
	index+= sizeof(msgtype);
	
	index = gfire_add_att_name(gfire->buff_out,index, "imindex");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &imindex, sizeof(imindex));
	index+= sizeof(imindex);

	index = gfire_add_att_name(gfire->buff_out,index, "typing");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;

	gfire_add_header(gfire->buff_out, index, 2, 2);

	return index;

}

void gfire_read_alias_change(PurpleConnection *gc, int packet_len)
{
	guint8 uid[XFIRE_USERID_LEN] = {0x0,0x0,0x0,0x0};
	gfire_data *gfire = NULL;
	int index = 7; /* start of uid in packet */
	guint16 nicklen = 0;
	gchar *nick = NULL;
	GList *gfbl = NULL;
	gfire_buddy *gbuddy = NULL;
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (index > packet_len)) return;

	/* grab the userid */
	memcpy(uid, gfire->buff_in + index, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN + 2;

	/* get the new nick len */
	memcpy(&nicklen, gfire->buff_in + index, sizeof(nicklen));
	nicklen = GUINT16_FROM_LE(nicklen);
	index += sizeof(nicklen);

	if (nicklen > 0) {
		/* grab the new nick */
		nick = g_malloc0(nicklen + 1);
		memcpy(nick, gfire->buff_in + index, nicklen);
	}
	
	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer) &uid, GFFB_UIDBIN);
	if (NULL == gfbl) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(nick change): uid not found in buddy list\n");
		if (NULL != nick) g_free(nick);
		return;
	}

	gbuddy = (gfire_buddy *)gfbl->data;
	if (NULL == gbuddy) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(nick change): uid found but gbuddy is {NULL}\n");
		if (NULL != nick) g_free(nick);
		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(nick change): user %s changed alias from %s to %s\n",
			NN(gbuddy->name), NN(gbuddy->alias), NN(nick));

	if (NULL != gbuddy->alias) g_free(gbuddy->alias);
	if (NULL != nick) {
		gbuddy->alias = nick;
	} else {
		gbuddy->alias = g_strdup(gbuddy->name);
	}

	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, gbuddy->name);
	
	if (NULL == buddy) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(nick change): purple_buddy_find returned null for %s\n",
				NN(gbuddy->name));
		return;
	}
	
	serv_got_alias(gc, gbuddy->name, gbuddy->alias);
	
}

int gfire_create_join_chat(PurpleConnection *gc, gchar *id, gchar *room, gchar *pass)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !room || (strlen(room) == 0)) {
	    
	    purple_debug(PURPLE_DEBUG_ERROR, "gfire",
					"(chat): invalid parameter to _create_join_chat gc=%p gfire=%p room=%s\n",
					gc, gfire, NN(room));
	    return 0;
	}
	
	
	slen = strlen(room);
	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0xf4;
	gfire->buff_out[index++] = 0x4c;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x06;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;
	memcpy(gfire->buff_out + index, id, XFIRE_CHATID_LEN);
	index+= XFIRE_CHATID_LEN;
	gfire->buff_out[index++] = 0x0b;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0xaa;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x05;
	gfire->buff_out[index++] = 0x01;
	slen = GUINT16_TO_LE(slen);
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, room, strlen(room));
	index += strlen(room);
	gfire->buff_out[index++] = 0x5f;
	gfire->buff_out[index++] = 0x01;
	if (!pass || strlen(pass) == 0) {
		gfire->buff_out[index++] = 0x00;
		gfire->buff_out[index++] = 0x00;
	} else {
		slen = GUINT16_TO_LE(slen);
		memcpy(gfire->buff_out + index, &slen, sizeof(slen));
		index += sizeof(slen);
		memcpy(gfire->buff_out + index, pass, strlen(pass));
		index += strlen(pass);
	}
	gfire->buff_out[index++] = 0xa7;
	gfire->buff_out[index++] = 0x08;
	gfire->buff_out[index++] = 0x00;
 
	gfire_add_header(gfire->buff_out, index, 25, 2);

	return index;
}


GList *gfire_read_chat_info(PurpleConnection *gc, int packet_len, gchar **rtop, gchar ** rmotd, guint8 **rcid)
{
	int index = XFIRE_HEADER_LEN + 2;
	int itmp = 0;
	gfire_data *gfire = NULL;
	guint16 slen = 0;
	guint8 chat_id[XFIRE_CHATID_LEN];
	guint32 i32_perm = 0;
	GList *members = NULL;
	gfire_buddy *m = NULL;
	gchar *topic = NULL;
	gchar *motd = NULL;
	GList *userids, *perms, *names, *nicks;
	GList *u, *p, *n, *a;

	u = p = n = a = NULL;
	userids = perms = names = nicks =  NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (packet_len == 0)) return NULL;

	/* xfire chat id */
	memcpy(chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;


	/* chat topic / name */
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	index += sizeof(slen);
	slen = GUINT16_FROM_LE(slen);
	if (slen > 0) {
		topic = g_malloc0(slen + 1);
		memcpy(topic, gfire->buff_in + index, slen);
	}
	index += slen + 2;


	/* Motd */
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	index += sizeof(slen);
	slen = GUINT16_FROM_LE(slen);
	if (slen > 0) {
		motd = g_malloc0(slen + 1);
		memcpy(motd, gfire->buff_in + index, slen);
	}
	index += slen + 27;

	/* get user id's of members in chat */
	itmp = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, NULL, FALSE, TRUE, 0, 0, 
							XFIRE_USERID_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
			return NULL;
	}	
	index += itmp + 3;


	/* get user permissions of members in chat */
	itmp = gfire_read_attrib(&perms, gfire->buff_in + index, packet_len - index, NULL, FALSE, TRUE, 0, 0, 
							sizeof(guint32));
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
		if (perms) g_list_free(perms);
			return NULL;
	}	
	index += itmp + 3;

	
	/* get chat member usernames */
	itmp = gfire_read_attrib(&names, gfire->buff_in + index, packet_len - index, NULL, TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
		if (perms) g_list_free(perms);
		if (names) g_list_free(names);
			return NULL;
	}	
	index += itmp + 3; /* 0x43 0x04 0x01 */

	/* get chat member aliases */
	itmp = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, NULL, TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		//mem cleanup code
		if (userids) g_list_free(userids);
		if (perms) g_list_free(perms);
		if (names) g_list_free(names);
		if (nicks) g_list_free(nicks);
			return NULL;
	}	

	/* packet has been parsed */
	names = g_list_first(names); nicks = g_list_first(nicks); userids = g_list_first(userids);
	perms = g_list_first(perms);

	n = names; p = perms; a = nicks; u = userids;

	while (NULL != u){
		m = g_new0(gfire_buddy,1);
		m->name = (gchar *)n->data;
		m->alias = (gchar *)a->data;
		m->userid = (guint8 *)u->data;
		m->groupchat = (gboolean )TRUE;
		memcpy(&i32_perm, p->data, sizeof(i32_perm));
		m->chatperm = GUINT32_FROM_LE(i32_perm);
		g_free(p->data); p->data = NULL;
		members = g_list_append(members, m);
		u->data = n->data = a->data = NULL;
		u = g_list_next(u); n = g_list_next(n);
		a = g_list_next(a); p = g_list_next(p);
	}

	g_list_free(names); g_list_free(perms); g_list_free(nicks); g_list_free(userids);

	*rtop = topic;
	*rmotd = motd;
	*rcid = g_malloc0(XFIRE_CHATID_LEN);
	memcpy(*rcid, chat_id, XFIRE_CHATID_LEN);
	return members;
}


gfire_chat_msg *gfire_read_chat_msg(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 2;
	gfire_chat_msg *ret = NULL;
	guint8 chat_id[XFIRE_CHATID_LEN];
	guint16 slen = 0;
	gchar *msg = NULL;
	guint8 userid[XFIRE_USERID_LEN];
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (packet_len == 0)) return NULL;
	
	memcpy(chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;

	if ((index +XFIRE_USERID_LEN )> packet_len) return NULL;
	
	memcpy(userid, gfire->buff_in + index, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN + 2;


	if ((index + sizeof(slen)) > packet_len) return NULL;
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	slen = GUINT16_FROM_LE(slen);
	index += sizeof(slen);

	if ((index + slen) > packet_len) return NULL;

	msg = g_malloc0(slen +1);
	memcpy(msg, gfire->buff_in + index, slen);
	
	ret = g_new0(gfire_chat_msg, 1);
	ret->im_str = msg;
	ret->chat_id = g_malloc0(XFIRE_CHATID_LEN);
	memcpy(ret->chat_id, chat_id, XFIRE_CHATID_LEN);
	ret->uid = g_malloc0(XFIRE_USERID_LEN);
	memcpy(ret->uid, userid, XFIRE_USERID_LEN);

	return ret;
}


int gfire_create_chat_message(PurpleConnection *gc, guint8 *cid, const char *message)
{
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;
	gfire_data *gfire = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)
		|| !message || !cid || (strlen(message) == 0)) return 0;

	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0xf6;
	gfire->buff_out[index++] = 0x4c;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;

	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;
	memcpy(gfire->buff_out + index, cid, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN;

	gfire->buff_out[index++] = 0x2e;
	gfire->buff_out[index++] = 0x01;

	slen = strlen(message);
	slen = GUINT16_TO_LE(slen);
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);

	memcpy(gfire->buff_out + index, message, strlen(message));
	index += strlen(message);

	gfire_add_header(gfire->buff_out, index, 25 , 0x02);

	return index;
}


gfire_chat_msg *gfire_read_chat_user_leave(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 2;
	gfire_data *gfire = NULL;
	gfire_chat_msg *ret = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (packet_len == 0)) return NULL;

	ret = g_new0(gfire_chat_msg,1);
	ret->chat_id = g_malloc0(XFIRE_CHATID_LEN);
	ret->uid = g_malloc0(XFIRE_USERID_LEN);

	memcpy(ret->chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;

	memcpy(ret->uid, gfire->buff_in + index, XFIRE_USERID_LEN);

	return ret;

}


int gfire_create_chat_leave(PurpleConnection *gc, const guint8 *cid)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !cid) return 0;

	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0xf5;
	gfire->buff_out[index++] = 0x4c;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;

	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;

	memcpy(gfire->buff_out + index, cid, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN;

	gfire_add_header(gfire->buff_out, index, 25 , 2);

	return index;
}


gfire_chat_msg *gfire_read_chat_user_join(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 2;
	gfire_chat_msg *ret = NULL;
	guint8 chat_id[XFIRE_CHATID_LEN];
	guint16 slen = 0;
	guint32 perms = 0;
	gchar *name = NULL;
	gchar *alias = NULL;
	guint8 userid[XFIRE_USERID_LEN];
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (packet_len == 0)) return NULL;

	memcpy(chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;

	if ((index + XFIRE_USERID_LEN) > packet_len) return NULL;
	memcpy(userid, gfire->buff_in + index, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN + 2;
	
	/* grab login ID */
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	slen = GUINT16_FROM_LE(slen);
	index += sizeof(slen);
	name = g_malloc0(slen + 1);
	memcpy(name, gfire->buff_in + index, slen);
	index += slen + 2;

	/* grab alias */
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	slen = GUINT16_FROM_LE(slen);
	index += sizeof(slen);
	if (slen > 0) {
		alias = g_malloc0(slen + 1);
		memcpy(alias, gfire->buff_in + index, slen);
	}
	index += slen + 2;

	/* grab perms for this chat user */
	memcpy(&perms, gfire->buff_in + index, sizeof(perms));
	perms = GUINT32_FROM_LE(perms);

	ret = g_new0(gfire_chat_msg, 1);
	ret->b = g_new0(gfire_buddy, 1);
	ret->chat_id = g_malloc0(XFIRE_CHATID_LEN);
	memcpy(ret->chat_id, chat_id, XFIRE_CHATID_LEN);
	ret->b->userid = g_malloc0(XFIRE_USERID_LEN);
	memcpy(ret->b->userid, userid, XFIRE_USERID_LEN);
	ret->b->chatperm = perms;
	ret->b->name = name;
	ret->b->alias = alias;
	ret->uid = NULL;
	ret->im_str = NULL;
		
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "groupchat join, userid: %02x %02x %02x %02x, username: %s, alias: %s\n",
		NNA(ret->b->userid, ret->b->userid[0]),
		NNA(ret->b->userid, ret->b->userid[1]), NNA(ret->b->userid, ret->b->userid[2]),
		NNA(ret->b->userid, ret->b->userid[3]), NN(ret->b->name), NN(ret->b->alias));
			
	return ret;
}


int gfire_create_chat_invite(PurpleConnection *gc, const guint8 *cid, const guint8 *userid)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !cid || !userid) return 0;

	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0xfc;
	gfire->buff_out[index++] = 0x4c;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;
	memcpy(gfire->buff_out + index, cid, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN;
	gfire->buff_out[index++] = 0x18;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x00;
	memcpy(gfire->buff_out + index, userid, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN;

	gfire_add_header(gfire->buff_out, index, 25 , 2);

	return index;
}

int gfire_create_change_motd(PurpleConnection *gc, const guint8 *cid, gchar* motd)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint16 slen = 0;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !cid 
		|| !motd || (strlen(motd) == 0)) return 0;

	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x0c;
	gfire->buff_out[index++] = 0x4d;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;
	memcpy(gfire->buff_out + index, cid, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN;
	gfire->buff_out[index++] = 0x2e;
	gfire->buff_out[index++] = 0x01;
	slen = strlen(motd);
	slen = GUINT16_TO_LE(slen);
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, motd, strlen(motd));
	index += strlen(motd);
	
	gfire_add_header(gfire->buff_out, index, 25 , 2);
	
	return index;		
}

void gfire_read_chat_motd_change(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 2;
	gfire_data *gfire = NULL;
	gchar *motd = NULL;
	guint16 slen = 0;
	GList *cl = NULL;
	guint8 chat_id[XFIRE_CHATID_LEN];
	gfire_chat *gfchat = NULL;
	gchar *tmpmsg = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (packet_len == 0)) return;

	memcpy(chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;
	
	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	index += sizeof(slen);
	slen = GUINT16_FROM_LE(slen);
	if (slen > 0) {
		motd = g_malloc0(slen + 1);
		memcpy(motd, gfire->buff_in + index, slen);
	}
	
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "new motd: %s\n", motd);
	
	cl = gfire_find_chat(gfire->chats, (gpointer *)chat_id, GFFC_CID);

	if (!cl || !(gfchat = (gfire_chat *)cl->data)) return;
	
	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(gfchat->c), "", motd);
	tmpmsg = g_strdup_printf("Today's message changed to:\n%s", motd);
	purple_conv_chat_write(PURPLE_CONV_CHAT(gfchat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
	g_free(tmpmsg);

	return;

}

void read_groupchat_buddy_permission_change(PurpleConnection *gc, int packet_len)
{
	guint8 uid[XFIRE_USERID_LEN] = {0x0,0x0,0x0,0x0};
	guint8 chat_id[XFIRE_CHATID_LEN];
	guint32 perm = 0;
	gfire_data *gfire = NULL;
	int index = 7; /* start of uid in packet */
	GList *gfbl = NULL;
	gfire_buddy *gbuddy = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	PurpleConvChatBuddyFlags f;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (index > packet_len)) return;

	/* grab the chat id */
	memcpy(chat_id, gfire->buff_in + index, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN + 2;

	/* grab the userid */
	memcpy(uid, gfire->buff_in + index, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN + 2;

	/* grab the new permission level */
	memcpy(&perm, gfire->buff_in + index, sizeof(perm));
	perm = GUINT32_FROM_LE(perm);

	
	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer) &uid, GFFB_UIDBIN);
	if (NULL == gfbl) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(groupchat perm change): uid not found in buddy list\n");
		if (0 != perm) perm = 0;
		return;
	}

	gbuddy = (gfire_buddy *)gfbl->data;
	if (NULL == gbuddy) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(groupchat perm change): uid found but gbuddy is {NULL}\n");
		if (0 != perm) perm = 0;
		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(groupchat perm change): user %s changed permission\n",
			NN(gbuddy->name));

	if (0 != gbuddy->chatperm) gbuddy->chatperm = 0;
	if (0 != perm) gbuddy->chatperm = perm;
	
	t = gfire_find_chat(gfire->chats, chat_id, GFFC_CID);
	if (t && (gfchat = (gfire_chat *)t->data)) {
		switch(gbuddy->chatperm) {
				case 01:
					f = PURPLE_CBFLAGS_NONE;
				break;
					
				case 02:
					f = PURPLE_CBFLAGS_NONE;
				break;
			
				case 03:
					f = PURPLE_CBFLAGS_VOICE;
				break;
			
				case 04:
					f = PURPLE_CBFLAGS_HALFOP;
				break;
			
				case 05:
					f = PURPLE_CBFLAGS_OP;
				break;
			
				default:
					f = PURPLE_CBFLAGS_NONE;
		}
	}
	
	purple_conv_chat_user_set_flags(PURPLE_CONV_CHAT(gfchat->c), gbuddy->name, f);
	
	return;
}

int gfire_create_reject_chat(PurpleConnection *gc, const guint8 *cid)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !cid) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "fail\n");
		return 0;
	}

	index = gfire_add_att_name(gfire->buff_out, index, "climsg");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0xff;
	gfire->buff_out[index++] = 0x4c;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	index = gfire_add_att_name(gfire->buff_out, index, "msg");
	gfire->buff_out[index++] = 0x09;
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x06;
	memcpy(gfire->buff_out + index, cid, XFIRE_CHATID_LEN);
	index += XFIRE_CHATID_LEN;

	gfire_add_header(gfire->buff_out, index, 25 , 2);
	
	return index;		
	
}

void gfire_read_clan_blist(PurpleConnection *gc, int packet_len)
{
	int index, itmp, i, type = 0;
	gfire_buddy *gf_buddy = NULL;
	guint8 clanid[XFIRE_CLANID_LEN] = {0x0,0x0,0x0,0x0};
	GList *userids = NULL;
	GList *usernames = NULL;
	GList *nicks = NULL;
	gchar uids[(XFIRE_USERID_LEN * 2) + 1];
	

	GList *f, *n, *u;
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	if (packet_len < 16) {
		purple_debug_error("gfire", "packet 131 received, but too short. (%d bytes)\n", packet_len);
		return;
	}

	index = 7;
	memcpy(clanid, gfire->buff_in + index, XFIRE_CLANID_LEN);
	index += XFIRE_CLANID_LEN + 3;
	
	itmp = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, NULL, FALSE, TRUE, 0, 0, XFIRE_USERID_LEN);
	if (itmp < 1) {
		if (userids != NULL) g_list_free(userids);
		return;
	}

	index += itmp + 3;
	itmp = gfire_read_attrib(&usernames, gfire->buff_in + index, packet_len - index, NULL, TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		if (userids != NULL) g_list_free(userids);
		if (usernames != NULL) g_list_free(usernames);
		return;
	}

	index += itmp + 3;	
	itmp = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, NULL, TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
		if (userids != NULL) g_list_free(userids);
		if (usernames != NULL) g_list_free(usernames);
		if (nicks != NULL) g_list_free(nicks);
		return;
	}

	userids = g_list_first(userids);
	usernames = g_list_first(usernames);
	nicks = g_list_first(nicks);
	
	u = userids;
	f = usernames;
	n = nicks;
	
	while (f != NULL)
	{
		gf_buddy = g_malloc0(sizeof(gfire_buddy));
		gfire->buddies = g_list_append(gfire->buddies, (gpointer *)gf_buddy);

		gf_buddy->name = (gchar *)f->data;
		
		/* No specific clan names for buddies who are already in our buddy list */
		if (!gf_buddy->friend) gf_buddy->alias = (gchar *)n->data;

		gf_buddy->userid = (guint8 *)u->data;
		gf_buddy->clanid = clanid;
		gf_buddy->clan = (gboolean )TRUE;

		if (gf_buddy->alias == NULL) gf_buddy->alias = g_strdup(gf_buddy->name);

		/* cast userid into string */
		for(i = 0; i < XFIRE_USERID_LEN; i++) g_sprintf(uids + (i * 2), "%02x", gf_buddy->userid[i]);

		uids[(XFIRE_USERID_LEN * 2)] = 0x00;
		gf_buddy->uid_str = g_strdup(uids);

		f->data = NULL;
		u->data = NULL;
		n->data = NULL;

		f = g_list_next(f);
		u = g_list_next(u);
		n = g_list_next(n);
	}

	g_list_free(userids);
	g_list_free(usernames);
	g_list_free(nicks);

	n = gfire->buddies;
	while (n != NULL)
	{
		gf_buddy = (gfire_buddy *)n->data;
		if (gf_buddy->clan) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "clan buddy info: %s, %02x%02x%02x%02x, %s, %02x%02x%02x%02x\n",
			     NN(gf_buddy->name), NNA(gf_buddy->userid, gf_buddy->userid[0]),
			     NNA(gf_buddy->userid, gf_buddy->userid[1]), NNA(gf_buddy->userid, gf_buddy->userid[2]),
			     NNA(gf_buddy->userid, gf_buddy->userid[3]), NN(gf_buddy->alias),
			     NNA(gf_buddy->clanid, gf_buddy->clanid[0]), NNA(gf_buddy->clanid, gf_buddy->clanid[1]),
			     NNA(gf_buddy->clanid, gf_buddy->clanid[2]), NNA(gf_buddy->clanid, gf_buddy->clanid[3]));
		}
		n = g_list_next(n);
	}
}


int gfire_create_serverlist_request (PurpleConnection *gc, int game)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint32 gameid = game;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	gameid = GUINT32_TO_LE(gameid);

	gfire->buff_out[index++] = 0x21;
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &gameid, sizeof(gameid));
	index += sizeof(gameid);

	gfire_add_header(gfire->buff_out, index, 22, 1);

	return index;
}

void gfire_read_serverlist(PurpleConnection *gc, int packet_len)
{
	gfire_data *gfire = NULL;
	int index, itmp = 0;
	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;
	GList *i, *p = NULL;
	guint8 *ip = NULL;
	guint32 port;
	
	GList *server_list = NULL;
	gchar *server;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	if (packet_len < 16) {
		purple_debug_error("gfire", "packet 131 received, but too short. (%d bytes)\n", packet_len);
		return;
	}

	index = 7;
	memcpy(&(gameid), gfire->buff_in + index, XFIRE_GAMEID_LEN);
	gameid = GUINT32_FROM_LE(gameid);
	index += XFIRE_GAMEID_LEN + 3;

	itmp = gfire_read_attrib(&ips, gfire->buff_in + index, packet_len - index, NULL, FALSE, TRUE, 0, 0, 
							XFIRE_GAMEIP_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (ips) g_list_free(ips);
			return NULL;
	}	
	index += itmp + 3;
	itmp = gfire_read_attrib(&ports, gfire->buff_in + index, packet_len - index, NULL, FALSE, TRUE, 0, 0, 
							XFIRE_GAMEPORT_LEN);
	if (itmp < 1 ) {
		//mem cleanup code
		if (ips) g_list_free(ips);
		if (ports) g_list_free(ports);
			return NULL;
	}

	ips = g_list_first(ips); ports = g_list_first(ports);
	i = ips; p = ports;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(serverlist): got the server list for %d\n", gameid);

	while (i != NULL)
	{
		ip = (guint8 *)i->data;
		memcpy(&(port),p->data, XFIRE_GAMEPORT_LEN);
		port = GUINT32_FROM_LE(port);
		port &= 0xFFFF;

		/* purple_debug(PURPLE_DEBUG_MISC, "gfire", "(serverlist): server: %d.%d.%d.%d:%d\n",
					NNA(ip, ip[3]), NNA(ip, ip[2]), NNA(ip, ip[1]), NNA(ip, ip[0]), port); */
		
		gchar *ipstr = g_strdup_printf("%d.%d.%d.%d", ip[3], ip[2], ip[1], ip[0]);
		gchar *server = g_strdup_printf("%s:%d", ipstr, port);
		server_list = g_list_append(server_list, server);		
		
		g_free(p->data);
		p->data = NULL;
		i = g_list_next(i);
		p = g_list_next(p);
	}

	gfire->server_list = server_list;
	server_list = gfire->server_list;
	
	GtkBuilder *builder = gfire->server_browser;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "servers_list_store"));
	
	GtkTreeIter iter;
	if (server_list != NULL)
	{
		server_list = g_list_first(server_list);
		while (server_list != NULL)
		{
			server_list = g_list_next(server_list);
			gtk_list_store_append(list_store, &iter);
			gtk_list_store_set(list_store, &iter, 0, server_list->data, 1, server_list->data, 2, "N/A", 3, "N/A", -1);
			server_list = g_list_next(server_list);
		}
	}
	else purple_debug_error("gfire_read_serverlist", "Couldn't get server list.\n");
}